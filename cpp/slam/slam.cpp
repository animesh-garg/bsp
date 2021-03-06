#include "slam.h"
#include "casadi/casadi-slam.h"

namespace py = boost::python;


const double step = 0.0078125*0.0078125;

std::vector< Matrix<P_DIM> > waypoints(NUM_WAYPOINTS);
std::vector< Matrix<P_DIM> > landmarks(NUM_LANDMARKS);

Matrix<X_DIM> x0;
Matrix<X_DIM,X_DIM> SqrtSigma0;
Matrix<X_DIM> xGoal;
Matrix<X_DIM> xMin, xMax;
Matrix<U_DIM> uMin, uMax;
SymmetricMatrix<Q_DIM> Q;
SymmetricMatrix<R_DIM> R;

CasADi::SXFunction casadi_belief_func;

const int T = TIMESTEPS;
const double INFTY = 1e10;

const std::string landmarks_file = "slam/landmarks.txt";


// ALWAYS call first. always.
void initProblemParams(std::vector<Matrix<P_DIM> >& l)
{
	Q.reset();
	Q(0,0) = 1*config::VELOCITY_NOISE*config::VELOCITY_NOISE;
	Q(1,1) = 1*config::TURNING_NOISE*config::TURNING_NOISE;

	R.reset();
	for(int i = 0; i < R_DIM-1; i+=2) {
		R(i,i) = 1*config::OBS_DIST_NOISE*config::OBS_DIST_NOISE; // 1
		R(i+1,i+1) = 1*config::OBS_ANGLE_NOISE*config::OBS_ANGLE_NOISE; // 1e-5
	}

	waypoints[0][0] = 60; waypoints[0][1] = 0;
	waypoints[1][0] = 60; waypoints[1][1] = 20;
	waypoints[2][0] = 0; waypoints[2][1] = 20;
	waypoints[3][0] = 0; waypoints[3][1] = 0;

	for(int i=0; i < NUM_LANDMARKS; ++i) {
		landmarks[i] = l[i];
	}
//	landmarks[0][0] = 30; landmarks[0][1] = -10;
//	landmarks[1][0] = 70; landmarks[1][1] = 12.5;
//	landmarks[2][0] = 20; landmarks[2][1] = 10;


	// start at (0, 0)
	// landmarks will be the same for all waypoint-waypoint optimizations
	x0.insert(0, 0, zeros<C_DIM,1>());

	for(int i = 0; i < NUM_LANDMARKS; ++i) {
		x0.insert(C_DIM+2*i, 0, landmarks[i]);
	}

	//This starts out at 0 for car, landmarks are set based on the car's sigma when first seen
	SqrtSigma0 = zeros<X_DIM, X_DIM>();
	for(int i = 0; i < C_DIM; ++i) { SqrtSigma0(i,i) = .1; } // .1
	for(int i = 0; i < L_DIM; ++i) { SqrtSigma0(C_DIM+i,C_DIM+i) = 3.5; } // 1

	uMin[0] = 1.5; // 1.5
	uMin[1] = -M_PI/3;
	uMax[0] = 10; // 10
	uMax[1] = M_PI/3;

	xMin[0] = -20;
	xMin[1] = -20;
	xMin[2] = -3*M_PI; // 2*M_PI
	for(int i=0; i < NUM_LANDMARKS; i++) {
		xMin[2*i+C_DIM] = landmarks[i][0] - 5;
		xMin[2*i+1+C_DIM] = landmarks[i][1] - 5;
	}

	xMax[0] = 80;
	xMax[1] = 80;
	xMax[2] = 3*M_PI; // 2*M_PI
	for(int i=0; i < NUM_LANDMARKS; i++) {
		xMax[2*i+C_DIM] = landmarks[i][0] + 5;
		xMax[2*i+1+C_DIM] = landmarks[i][1] + 5;
	}

	//casadi_belief_func = casadiBeliefDynamicsFunc();

}

std::vector<std::vector<Matrix<P_DIM>> > landmarks_list() {
	std::vector<std::vector<Matrix<P_DIM>> > l_list;

	std::string line;
	std::ifstream l_file (landmarks_file);
	if (l_file.is_open()) {
		int iter = 0;
		while(std::getline(l_file, line)) {
			if (iter < 1) { // ignore first line, which is time identifier
				iter++;
				continue;
			}
			std::vector<Matrix<P_DIM>> l(NUM_LANDMARKS);
			int index = 0;

			std::istringstream iss(line);
			double pos;
			while (iss >> pos) {
				l[index / P_DIM][index % P_DIM] = pos;
				index++;
			}
			l_list.push_back(l);
		}
	} else {
		LOG_ERROR("Couldn't open landmarks.txt!");
		exit(-1);
	}

	return l_list;
}

Matrix<X_DIM> dynfunc(const Matrix<X_DIM>& x, const Matrix<U_DIM>& u, const Matrix<Q_DIM>& q)
{
	Matrix<X_DIM> xAdd = zeros<X_DIM,1>();

	xAdd[0] = (u[0]+q[0]) * DT * cos(x[2]+u[1]+q[1]);
	xAdd[1] = (u[0]+q[0]) * DT * sin(x[2]+u[1]+q[1]);
	xAdd[2] = (u[0]+q[0]) * DT * sin(u[1]+q[1])/config::WHEELBASE;

	Matrix<X_DIM> xNew = x + xAdd;
    return xNew;
}

Matrix<C_DIM> dynfunccar(const Matrix<C_DIM>& x, const Matrix<U_DIM>& u)
{
	Matrix<C_DIM> xAdd = zeros<C_DIM,1>();

	xAdd[0] = u[0] * DT * cos(x[2]+u[1]);
	xAdd[1] = u[0] * DT * sin(x[2]+u[1]);
	xAdd[2] = u[0] * DT * sin(u[1])/config::WHEELBASE;

	Matrix<C_DIM> xNew = x + xAdd;
    return xNew;
}


Matrix<Z_DIM> obsfunc(const Matrix<X_DIM>& x, const Matrix<R_DIM>& r)
{
	double xPos = x[0], yPos = x[1], angle = x[2];
	double dx, dy;

	Matrix<Z_DIM> obs = zeros<Z_DIM,1>();

	for(int i = 0; i < L_DIM; i += 2) {
		dx = x[C_DIM+i] - xPos;
		dy = x[C_DIM+i+1] - yPos;

		//if ((fabs(dx) < config::MAX_RANGE) &&
		//	(fabs(dy) < config::MAX_RANGE) &&
		//	(dx*cos(angle) + dy*sin(angle) > 0) &&
		//	(dx*dx + dy*dy < config::MAX_RANGE*config::MAX_RANGE))
		//{
		obs[i] = sqrt(dx*dx + dy*dy) + r[i];
		obs[i+1] = atan2(dy, dx) - angle + r[i+1];
		//}
	}

	return obs;
}

Matrix<Z_DIM,Z_DIM> deltaMatrix(const Matrix<X_DIM>& x) {
	Matrix<Z_DIM,Z_DIM> delta = zeros<Z_DIM,Z_DIM>();
	double l0, l1, dist;
	for(int i=C_DIM; i < X_DIM; i += 2) {
		l0 = x[i];
		l1 = x[i+1];

		dist = sqrt((x[0] - l0)*(x[0] - l0) + (x[1] - l1)*(x[1] - l1));

		double signed_dist = 1/(1+exp(-config::ALPHA_OBS*(config::MAX_RANGE-dist)));
		delta(i-C_DIM,i-C_DIM) = signed_dist;
		delta(i-C_DIM+1,i-C_DIM+1) = signed_dist;
	}

	return delta;
}


// Jacobians: df(x,u,q)/dx, df(x,u,q)/dq
void linearizeDynamics(const Matrix<X_DIM>& x, const Matrix<U_DIM>& u, const Matrix<Q_DIM>& q, Matrix<C_DIM,C_DIM>& A, Matrix<C_DIM,Q_DIM>& M)
{
	//g is control input steer angle
	double s= sin(u[1]+x[2]); double c= cos(u[1]+x[2]);
	double vts= u[0]*DT*s; double vtc= u[0]*DT*c;

	M.reset();
	M(0, 0) = DT*c;
	M(0, 1) = -vts;
	M(1, 0) = DT*s;
	M(1, 1) = vtc;
	M(2, 0) = DT*sin(u[1])/config::WHEELBASE;
	M(2, 1) = u[0]*DT*cos(u[1])/config::WHEELBASE;

	A.reset();
	A(0,0) = 1;
	A(1,1) = 1;
	A(2,2) = 1;
	A(0,2) = -vts;
	A(1,2) = vtc;

}

// Jacobians: df(x,u,q)/dx, df(x,u,q)/du
void linearizeDynamicsFiniteDiff(const Matrix<X_DIM>& x, const Matrix<U_DIM>& u, const Matrix<Q_DIM>& q, Matrix<X_DIM,X_DIM>& A, Matrix<X_DIM,Q_DIM>& M)
{
	A.reset();
	Matrix<X_DIM> xr(x), xl(x);
	for (size_t i = 0; i < X_DIM; ++i) {
		xr[i] += step; xl[i] -= step;
		A.insert(0,i, (dynfunc(xr, u, zeros<Q_DIM,1>()) - dynfunc(xl, u,  zeros<Q_DIM,1>())) / (xr[i] - xl[i]));
		xr[i] = x[i]; xl[i] = x[i];
	}

	M.reset();
	Matrix<Q_DIM> qr(q), ql(q);
	for (size_t i = 0; i < Q_DIM; ++i) {
		qr[i] += step; ql[i] -= step;
		M.insert(0,i, (dynfunc(x, u, qr) - dynfunc(x, u, ql)) / (qr[i] - ql[i]));
		qr[i] = q[i]; ql[i] = q[i];
	}
}


// Jacobians: dh(x,r)/dx, dh(x,r)/dr
void linearizeObservation(const Matrix<X_DIM>& x, const Matrix<R_DIM>& r, Matrix<Z_DIM,X_DIM>& H, Matrix<Z_DIM,R_DIM>& N)
{

	H.reset();
	for (int i=0; i < L_DIM; i+=2) {
		double dx = x[C_DIM+i] - x[0];
		double dy = x[C_DIM+i+1] - x[1];
		double d2 = dx*dx + dy*dy + 1e-10;
		double d = sqrt(d2 + 1e-10);
		double xd = dx/d;
		double yd = dy/d;
		double xd2 = dx/d2;
		double yd2 = dy/d2;

		H(i, 0) = -xd;
		H(i, 1) = -yd;
		H(i, 2) = 0;
		H(i+1, 0) = yd2;
		H(i+1, 1) = -xd2;
		H(i+1, 2) = -1;
		H(i, 3+i) = xd;
		H(i, 3+i+1) = yd;
		H(i+1, 3+i) = -yd2;
		H(i+1, 3+i+1) = xd2;
	}

	N = identity<Z_DIM>();

}


// Jacobians: dh(x,r)/dx, dh(x,r)/dr
void linearizeObservationFiniteDiff(const Matrix<X_DIM>& x, const Matrix<R_DIM>& r, Matrix<Z_DIM,X_DIM>& H, Matrix<Z_DIM,R_DIM>& N)
{

	H.reset();
	Matrix<X_DIM> xr(x), xl(x);
	for (size_t i = 0; i < X_DIM; ++i) {
	  xr[i] += step; xl[i] -= step;
	  H.insert(0,i, (obsfunc(xr, r) - obsfunc(xl, r)) / (xr[i] - xl[i]));
	  xr[i] = x[i]; xl[i] = x[i];
	}


	N.reset();
	Matrix<R_DIM> rr(r), rl(r);
	for (size_t i = 0; i < R_DIM; ++i) {
		rr[i] += step; rl[i] -= step;
		N.insert(0,i, (obsfunc(x, rr) - obsfunc(x, rl)) / (rr[i] - rl[i]));
		rr[i] = r[i]; rl[i] = r[i];

	}

}


// Switch between belief vector and matrices
void unVec(const Matrix<B_DIM>& b, Matrix<X_DIM>& x, Matrix<X_DIM,X_DIM>& SqrtSigma) {
	x = b.subMatrix<X_DIM,1>(0,0);
	size_t idx = X_DIM;
	for (size_t j = 0; j < X_DIM; ++j) {
		for (size_t i = j; i < X_DIM; ++i) {
			SqrtSigma(i,j) = b[idx];
			SqrtSigma(j,i) = b[idx];
			++idx;
		}
	}
}

void vec(const Matrix<X_DIM>& x, const Matrix<X_DIM,X_DIM>& SqrtSigma, Matrix<B_DIM>& b) {
	b.insert(0,0,x);
	size_t idx = X_DIM;
	for (size_t j = 0; j < X_DIM; ++j) {
		for (size_t i = j; i < X_DIM; ++i) {
			b[idx] = 0.5 * (SqrtSigma(i,j) + SqrtSigma(j,i));
			++idx;
		}
	}
}

// Belief dynamics
Matrix<B_DIM> beliefDynamics(const Matrix<B_DIM>& b, const Matrix<U_DIM>& u) {
	Matrix<X_DIM> x;
	Matrix<X_DIM,X_DIM> SqrtSigma;
	unVec(b, x, SqrtSigma);

	Matrix<X_DIM,X_DIM> Sigma = SqrtSigma*SqrtSigma;

	Matrix<C_DIM,C_DIM> Acar;
	Matrix<C_DIM,Q_DIM> Mcar;
	linearizeDynamics(x, u, zeros<Q_DIM,1>(), Acar, Mcar);

	Matrix<X_DIM,X_DIM> A = identity<X_DIM>();
	A.insert<C_DIM,C_DIM>(0, 0, Acar);
	Matrix<X_DIM,Q_DIM> M = zeros<X_DIM,Q_DIM>();
	M.insert<C_DIM, 2>(0, 0, Mcar);

	Sigma = A*Sigma*~A + M*Q*~M;

	x = dynfunc(x, u, zeros<Q_DIM,1>());

	Matrix<Z_DIM,X_DIM> H;
	Matrix<Z_DIM,R_DIM> N;
	linearizeObservation(x, zeros<R_DIM,1>(), H, N);
	//Should include an R here

	Matrix<Z_DIM,Z_DIM> delta = deltaMatrix(x);

	Matrix<X_DIM,Z_DIM> K = ((Sigma*~H*delta)/(delta*H*Sigma*~H*delta + R))*delta;

	Sigma = (identity<X_DIM>() - K*H)*Sigma;

	SymmetricMatrix<X_DIM> SymmetricSigma, SqrtSymmetricSigma;
	for(int i=0; i < X_DIM; ++i) {
		for(int j=0; j < X_DIM; ++j) {
		SymmetricSigma(i,j) = Sigma(i,j);
		}
	}
	SqrtSymmetricSigma = sqrt(SymmetricSigma);
	Matrix<X_DIM,X_DIM> SqrtSigma_tp1;
	for(int i=0; i < X_DIM; ++i) {
		for(int j=0; j < X_DIM; ++j) {
			SqrtSigma_tp1(i,j) = SqrtSymmetricSigma(i,j);
		}
	}

	Matrix<B_DIM> g;
	vec(x, SqrtSigma_tp1, g);

//	Matrix<B_DIM> g;
//	vec(x, sqrtm(Sigma), g);

	return g;
}

Matrix<B_DIM> casadiBeliefDynamics(const Matrix<B_DIM>& b, const Matrix<U_DIM>& u) {
	casadi_belief_func.setInput(b.getPtr(),0);
	casadi_belief_func.setInput(u.getPtr(),1);
	casadi_belief_func.evaluate();

	Matrix<X_DIM+(X_DIM*X_DIM)> b_fullsigma_tp1;
	casadi_belief_func.getOutput(b_fullsigma_tp1.getPtr(),0);

	Matrix<X_DIM> x_tp1;
	SymmetricMatrix<X_DIM> Sigma_tp1;

	for(int i=0; i < X_DIM; ++i) { x_tp1[i] = b_fullsigma_tp1[i]; }
	int index = X_DIM;
	for (int j = 0; j < X_DIM; ++j) {
		for (int i = 0; i < X_DIM; ++i) {
			Sigma_tp1(i,j) = b_fullsigma_tp1(index++,0);
		}
	}

	SymmetricMatrix<X_DIM> SymmetricSqrtSigma_tp1 = sqrt(Sigma_tp1);
	Matrix<X_DIM,X_DIM> SqrtSigma_tp1;
	for(int i=0; i < X_DIM; ++i) {
		for(int j=0; j < X_DIM; ++j) {
			SqrtSigma_tp1(i,j) = SymmetricSqrtSigma_tp1(i,j);
		}
	}
	Matrix<B_DIM> b_tp1;
	vec(x_tp1, SqrtSigma_tp1, b_tp1);

	return b_tp1;
}

int numberLandmarksInRange(const Matrix<X_DIM>& x) {
	int num_obs = 0;
	double xPos = x[0], yPos = x[1], angle = x[2];

	for (int i=0; i < L_DIM; i+=2) {
		double dx = x[C_DIM+i] - xPos;
		double dy = x[C_DIM+i+1] - yPos;

		if ((fabs(dx) < config::MAX_RANGE) &&
				(fabs(dy) < config::MAX_RANGE) &&
				(dx*cos(angle) + dy*sin(angle) > 0) &&
				(dx*dx + dy*dy < config::MAX_RANGE*config::MAX_RANGE))
		{
			num_obs++;
		}
	}

	return num_obs;
}

template <size_t _z_dim_observed>
void filteredLinearizeObservation(const Matrix<X_DIM>& x, Matrix<_z_dim_observed,X_DIM>& H, Matrix<_z_dim_observed,_z_dim_observed>& N) {
	Matrix<Z_DIM,X_DIM> Hfull;
	Matrix<Z_DIM,R_DIM> Nfull;

	linearizeObservation(x, zeros<R_DIM,1>(), Hfull, Nfull);

	int num_obs = 0;
	double xPos = x[0], yPos = x[1], angle = x[2];

	for (int i=0; i < L_DIM; i+=2) {
		double dx = x[C_DIM+i] - xPos;
		double dy = x[C_DIM+i+1] - yPos;

		if ((fabs(dx) < config::MAX_RANGE) &&
				(fabs(dy) < config::MAX_RANGE) &&
				(dx*cos(angle) + dy*sin(angle) > 0) &&
				(dx*dx + dy*dy < config::MAX_RANGE*config::MAX_RANGE))
		{
			H.insert(2*num_obs, 0, Hfull.subMatrix<1,X_DIM>(i,0));
		}
	}

	N = identity<_z_dim_observed>(); // s
}

// Belief dynamics
Matrix<B_DIM> beliefDynamicsNoDelta(const Matrix<B_DIM>& b, const Matrix<U_DIM>& u) {
	Matrix<X_DIM> x;
	Matrix<X_DIM,X_DIM> SqrtSigma;
	unVec(b, x, SqrtSigma);

	Matrix<X_DIM,X_DIM> Sigma = SqrtSigma*SqrtSigma;

	Matrix<C_DIM,C_DIM> Acar;
	Matrix<C_DIM,Q_DIM> Mcar;
	linearizeDynamics(x, u, zeros<Q_DIM,1>(), Acar, Mcar);

	Matrix<X_DIM,X_DIM> A = identity<X_DIM>();
	A.insert<C_DIM,C_DIM>(0, 0, Acar);
	Matrix<X_DIM,Q_DIM> M = zeros<X_DIM,Q_DIM>();
	M.insert<C_DIM, 2>(0, 0, Mcar);

	Sigma = A*Sigma*~A + M*Q*~M;

	x = dynfunc(x, u, zeros<Q_DIM,1>());

	int z_dim_observed = numberLandmarksInRange(x);
	if (z_dim_observed > 0) {
		if (z_dim_observed == 1) {
			Matrix<P_DIM, X_DIM> H;
			Matrix<P_DIM, P_DIM> N;
			filteredLinearizeObservation(x, H, N);

			Matrix<X_DIM,P_DIM> K = (Sigma*~H)/(H*Sigma*~H + R.subSymmetricMatrix<P_DIM>(0));
			Sigma = (identity<X_DIM>() - K*H)*Sigma;
		} else if (z_dim_observed == 2) {
			Matrix<2*P_DIM, X_DIM> H;
			Matrix<2*P_DIM, 2*P_DIM> N;
			filteredLinearizeObservation(x, H, N);

			Matrix<X_DIM,2*P_DIM> K = (Sigma*~H)/(H*Sigma*~H + R.subSymmetricMatrix<2*P_DIM>(0));
			Sigma = (identity<X_DIM>() - K*H)*Sigma;
		} else if (z_dim_observed == 3) {
			Matrix<3*P_DIM, X_DIM> H;
			Matrix<3*P_DIM, 3*P_DIM> N;
			filteredLinearizeObservation(x, H, N);

			Matrix<X_DIM,3*P_DIM> K = (Sigma*~H)/(H*Sigma*~H + R.subSymmetricMatrix<3*P_DIM>(0));
			Sigma = (identity<X_DIM>() - K*H)*Sigma;
		}
	}

	Matrix<B_DIM> g;
	vec(x, sqrtm(Sigma), g);

	return g;
}

// returns updated belief based on real current state, estimated current belief, and input
void executeControlStep(const Matrix<X_DIM>& x_t_real, const Matrix<B_DIM>& b_t_t, const Matrix<U_DIM>& u_t, Matrix<X_DIM>& x_tp1_real, Matrix<B_DIM>& b_tp1_tp1) {
	// find next real state from input + noise
	// TODO: sample from smaller Q and R and try with zero noise
	Matrix<Q_DIM> control_noise = sampleGaussian(zeros<Q_DIM,1>(), .2*Q);
	std::cout << "control_noise: " << ~control_noise;
	x_tp1_real = dynfunc(x_t_real, u_t, control_noise);
	// sense real state + noise
	Matrix<R_DIM> obs_noise = sampleGaussian(zeros<R_DIM,1>(), .2*R);
	std::cout << "obs_noise: " << ~obs_noise;
	Matrix<Z_DIM> z_tp1_real = obsfunc(x_tp1_real, obs_noise);


	// now do update based on current belief (b_t_t)
	Matrix<X_DIM> x_t_t;
	Matrix<X_DIM,X_DIM> SqrtSigma_t_t;
	unVec(b_t_t, x_t_t, SqrtSigma_t_t);

	Matrix<X_DIM,X_DIM> Sigma_t_t = SqrtSigma_t_t*SqrtSigma_t_t;

	// linearize dynamics
	Matrix<C_DIM,C_DIM> Acar;
	Matrix<C_DIM,Q_DIM> Mcar;
	linearizeDynamics(x_t_t, u_t, zeros<Q_DIM,1>(), Acar, Mcar);

	Matrix<X_DIM,X_DIM> A = identity<X_DIM>();
	A.insert<C_DIM,C_DIM>(0, 0, Acar);
	Matrix<X_DIM,Q_DIM> M = zeros<X_DIM,Q_DIM>();
	M.insert<C_DIM, 2>(0, 0, Mcar);

	// dynamics update (maximum likelihood)
	Matrix<X_DIM> x_tp1_t = dynfunc(x_t_t, u_t, zeros<Q_DIM,1>());
	Matrix<X_DIM,X_DIM> Sigma_tp1_t = A*Sigma_t_t*~A + M*Q*~M;

	// linearize observation
	Matrix<Z_DIM,X_DIM> H;
	Matrix<Z_DIM,R_DIM> N;
	linearizeObservation(x_tp1_t, zeros<R_DIM,1>(), H, N);

	Matrix<Z_DIM,Z_DIM> delta = deltaMatrix(x_tp1_t);

	// calculate Kalman gain
	Matrix<X_DIM,Z_DIM> K = ((Sigma_tp1_t*~H*delta)/(delta*H*Sigma_tp1_t*~H*delta + R))*delta;

	// update based on noisy measurement
	Matrix<X_DIM> x_tp1_tp1 = x_tp1_t + K*(z_tp1_real - obsfunc(x_tp1_t, zeros<R_DIM,1>()));
	Matrix<X_DIM,X_DIM> Sigma_tp1_tp1 = (identity<X_DIM>() - K*H)*Sigma_tp1_t;

	vec(x_tp1_tp1, sqrtm(Sigma_tp1_tp1), b_tp1_tp1);
}

// Jacobians: dg(b,u)/db, dg(b,u)/du
void linearizeBeliefDynamics(const Matrix<B_DIM>& b, const Matrix<U_DIM>& u, Matrix<B_DIM,B_DIM>& F, Matrix<B_DIM,U_DIM>& G, Matrix<B_DIM>& h)
{
	F.reset();
	Matrix<B_DIM> br(b), bl(b);
	for (size_t i = 0; i < B_DIM; ++i) {
		br[i] += step; bl[i] -= step;
		F.insert(0,i, (beliefDynamics(br, u) - beliefDynamics(bl, u)) / (br[i] - bl[i]));
		br[i] = b[i]; bl[i] = b[i];
	}

	G.reset();
	Matrix<U_DIM> ur(u), ul(u);
	for (size_t i = 0; i < U_DIM; ++i) {
		ur[i] += step; ul[i] -= step;
		G.insert(0,i, (beliefDynamics(b, ur) - beliefDynamics(b, ul)) / (ur[i] - ul[i]));
		ur[i] = u[i]; ul[i] = u[i];
	}

	h = beliefDynamics(b, u);
}

// returns opened file handle for logging data
// writes first line, which is the number of landmarks
void logDataHandle(std::string file_name, std::ofstream& f) {
	std::string landmarks_identifier;
	std::ifstream l_file (landmarks_file);
	if (l_file.is_open()) {
		if (std::getline(l_file, landmarks_identifier)) {

		} else {
			LOG_ERROR("Couldn't read landmarks_file");
			return;
		}
	} else {
		LOG_ERROR("Couldn't open landmarks_file");
		return;
	}

	std::string file_name_with_identifier = file_name + "_" + landmarks_identifier + ".txt";
	f.open(file_name_with_identifier.c_str());
	f << NUM_LANDMARKS << "\n";
}

void logDataToFile(std::ofstream& f, const std::vector<Matrix<B_DIM> >& B, double solve_time, double initialization_time, double failure) {
	double sum_cov_trace = 0;
	Matrix<X_DIM> x;
	Matrix<X_DIM,X_DIM> SqrtSigma;
	for(size_t i=0; i < B.size(); ++i) {
		unVec(B[i], x, SqrtSigma);
		sum_cov_trace += tr(SqrtSigma*SqrtSigma);
	}

	// assuming only one loop around
	double waypoint_distance_error = 0;
	for(int w_idx=0; w_idx < NUM_WAYPOINTS; ++w_idx) {
		Matrix<P_DIM> w = waypoints[w_idx];
		double min_dist = INFTY;
		for(size_t i=0; i < B.size(); ++i) {
			unVec(B[i], x, SqrtSigma);
			Matrix<P_DIM> diff = w - x.subMatrix<P_DIM,1>(0,0);
			min_dist = MIN(min_dist, sqrt(tr(~diff*diff)));
		}
		waypoint_distance_error += min_dist;
	}


	if (f.is_open()) {
		f << "sum_cov_trace " << sum_cov_trace << "\n";
		f << "waypoint_distance_error " << waypoint_distance_error << "\n";
		f << "solve_time " << solve_time << "\n";
		f << "initialization_time " << initialization_time << "\n";
		f << "failure " << failure << "\n";
	} else {
		LOG_ERROR("Couldn't write in logDataToFile");
		return;
	}
}

void pythonDisplayTrajectory(std::vector< Matrix<X_DIM> >& X, int time_steps, bool pause) {
	std::vector<Matrix<B_DIM> > B(time_steps);
	std::vector<Matrix<U_DIM> > U(time_steps-1,zeros<U_DIM,1>());

	for (int t = 0; t < time_steps; ++t) {
		vec(X[t], zeros<X_DIM,X_DIM>(), B[t]);
	}

	pythonDisplayTrajectory(B, U, waypoints, landmarks, time_steps, pause);
}


void pythonDisplayTrajectory(std::vector< Matrix<U_DIM> >& U, int time_steps, bool pause) {
	std::vector<Matrix<B_DIM> > B(time_steps);

	vec(x0, SqrtSigma0, B[0]);
	for (int t = 0; t < time_steps-1; ++t) {
		B[t+1] = beliefDynamics(B[t], U[t]);
	}

	pythonDisplayTrajectory(B, U, waypoints, landmarks, time_steps, pause);
}

void pythonDisplayTrajectory(std::vector< Matrix<B_DIM> >& B, std::vector< Matrix<U_DIM> >& U, std::vector< Matrix<P_DIM> >& waypoints, std::vector< Matrix<P_DIM> >& landmarks, int time_steps, bool pause)
{

	py::list B_vec;
	for(int j=0; j < B_DIM; j++) {
		for(int i=0; i < time_steps; i++) {
			B_vec.append(B[i][j]);
		}
	}


	py::list U_vec;
	for(int j=0; j < U_DIM; j++) {
		for(int i=0; i < time_steps-1; i++) {
			U_vec.append(U[i][j]);
		}
	}

	py::list waypoints_vec;
	for(int j=0; j < 2; j++) {
		for(int i=0; i < NUM_WAYPOINTS; i++) {
			waypoints_vec.append(waypoints[i][j]);
		}
	}

	py::list landmarks_vec;
	for(int j=0; j < 2; j++) {
		for(int i=0; i < NUM_LANDMARKS; i++) {
			landmarks_vec.append(landmarks[i][j]);
		}
	}

	//std::string workingDir = boost::filesystem::current_path().normalize().string();
	//std::string workingDir = "/home/gkahn/bsp/cpp";
	std::string workingDir = "/home/gkahn/Research/bsp/cpp";

	try
	{
		Py_Initialize();
		py::object main_module = py::import("__main__");
		py::object main_namespace = main_module.attr("__dict__");
		py::exec("import sys, os", main_namespace);
		py::exec(py::str("sys.path.append('"+workingDir+"/slam')"), main_namespace);
		py::object plot_mod = py::import("plot_point_slam");
		py::object plot_traj = plot_mod.attr("plot_point_trajectory");

		plot_traj(B_vec, U_vec, waypoints_vec, landmarks_vec, config::MAX_RANGE, config::ALPHA_OBS, X_DIM, time_steps, DT);

		if (pause) {
			LOG_INFO("Press enter to continue");
			py::exec("raw_input()",main_namespace);
			//std::cin.ignore();
		}
	}
	catch(py::error_already_set const &)
	{
		PyErr_Print();
	}

}


