#include "../parameter.h"

#include "util/matrix.h"

#include "ilqg.h"

#include <time.h>

#include <Python.h>
#include <boost/python.hpp>
#include <boost/filesystem.hpp>

namespace py = boost::python;


SymmetricMatrix<U_DIM> Rint;
SymmetricMatrix<X_DIM> Qint;
SymmetricMatrix<X_DIM> QGoal, QGoalVariance, QintVariance;
SymmetricMatrix<X_DIM> Sigma0;

Matrix<X_DIM> x0, xGoal;

inline Matrix<X_DIM> f(const Matrix<X_DIM>& x, const Matrix<U_DIM>& u)
{
	return dynfunc(x, u, zeros<U_DIM,1>());
}

// Observation model
inline Matrix<Z_DIM> h(const Matrix<X_DIM>& x)
{
	return obsfunc(x, zeros<Z_DIM,1>());
}

// Jacobian df/dx(x,u)
template <size_t _xDim, size_t _uDim>
inline Matrix<_xDim,_xDim> dfdx(Matrix<_xDim> (*f)(const Matrix<_xDim>&, const Matrix<_uDim>&), const Matrix<_xDim>& x, const Matrix<_uDim>& u) 
{
	Matrix<_xDim,_xDim> A;
	Matrix<_xDim> xr(x), xl(x);
	for (size_t i = 0; i < _xDim; ++i) {
		xr[i] += step; xl[i] -= step;
		A.insert(0,i, (f(xr, u) - f(xl, u)) / (2.0*step));
		xr[i] = xl[i] = x[i];
	}
	return A;
}

// Jacobian df/du(x,u)
template <size_t _xDim, size_t _uDim>
inline Matrix<_xDim,_uDim> dfdu(Matrix<_xDim> (*f)(const Matrix<_xDim>&, const Matrix<_uDim>&),	const Matrix<_xDim>& x, const Matrix<_uDim>& u) 
{
	Matrix<_xDim,_uDim> B;
	Matrix<_uDim> ur(u), ul(u);
	for (size_t i = 0; i < _uDim; ++i) {
		ur[i] += step; ul[i] -= step;
		B.insert(0,i, (f(x, ur) - f(x, ul)) / (2.0*step));
		ur[i] = ul[i] = u[i];
	}
	return B;
}

// Jacobian dh/dx(x)
template <size_t _xDim, size_t _zDim>
inline Matrix<_zDim,_xDim> dhdx(Matrix<_zDim> (*h)(const Matrix<_xDim>&), const Matrix<_xDim>& x) 
{
	Matrix<_zDim,_xDim> H;
	Matrix<_xDim> xr(x), xl(x);
	for (size_t i = 0; i < _xDim; ++i) {
		xr[i] += step; xl[i] -= step;
		H.insert(0,i, (h(xr) - h(xl)) / (2.0*step));
		xr[i] = xl[i] = x[i];
	}
	return H;
}

inline SymmetricMatrix<X_DIM> varM(const Matrix<X_DIM>& x, const Matrix<U_DIM>& u)
{
	SymmetricMatrix<X_DIM> S = identity<X_DIM>();
	S(0,0) = 0.25*0.001 + 0.0000000000001;
	S(1,1) = 0.25*0.001 + 0.0000000000001;
	S(2,2) = 1.0*0.001 + 0.0000000000001;
	S(3,3) = 1.0*0.001 + 0.0000000000001;
	S(4,4) = 0.0005;
	S(5,5) = 0.0005;
	S(6,6) = 0.0001;
	S(7,7) = 0.0001;
	return S;
}

inline SymmetricMatrix<Z_DIM> varN(const Matrix<X_DIM>& x)
{
	SymmetricMatrix<Z_DIM> S = identity<Z_DIM>();
	S(0,0) = 0.0001;
	S(1,1) = 0.0001;
	S(2,2) = 0.00001;
	S(3,3) = 0.00001;
	return S;
}

// Compute closed form quadratic finalCost function around around b
inline void quadratizeFinalCost(const Matrix<X_DIM>& xBar, const SymmetricMatrix<X_DIM>& SigmaBar, double& s, SymmetricMatrix<X_DIM>& S, Matrix<1,X_DIM>& sT, Matrix<1,S_DIM>& tT, unsigned int flag) 
{
	if (flag & COMPUTE_S) S = QGoal;
	if (flag & COMPUTE_sT) sT = ~(xBar - xGoal)*QGoal;
	if (flag & COMPUTE_s) s = 0.5*scalar(~(xBar - xGoal)*QGoal*(xBar - xGoal)) + scalar(vecTh(QGoalVariance)*vectorize(SigmaBar));
	if (flag & COMPUTE_tT) tT = vecTh(QGoalVariance);
}

// Compute closed form quadratic cost function around around b and u
inline bool quadratizeCost(const Matrix<X_DIM>& xBar, const SymmetricMatrix<X_DIM>& SigmaBar, const Matrix<U_DIM>& uBar, double& q, SymmetricMatrix<X_DIM>& Q, SymmetricMatrix<U_DIM>& R, 
						   Matrix<U_DIM, X_DIM>& P, Matrix<1,X_DIM>& qT, Matrix<1,U_DIM>& rT, Matrix<1,S_DIM>& pT, unsigned int flag) 
{
	if (flag & COMPUTE_Q) Q = Qint; // penalize uncertainty
	if (flag & COMPUTE_R) R = Rint;
	if (flag & COMPUTE_P) P = zeros<U_DIM,X_DIM>();
	if (flag & COMPUTE_qT) qT = ~(xBar - xGoal)*Qint;
	if (flag & COMPUTE_rT) rT = ~uBar*Rint;
	if (flag & COMPUTE_pT) pT = vecTh(QintVariance);
	if (flag & COMPUTE_q) q = 0.5*scalar(~(xBar - xGoal)*Qint*(xBar - xGoal)) + 0.5*scalar(~uBar*Rint*uBar) + scalar(vecTh(QintVariance)*vectorize(SigmaBar));
	return true;
}

inline void linearizeDynamics(const Matrix<X_DIM>& xBar, const Matrix<U_DIM>& uBar, Matrix<X_DIM>& c, Matrix<X_DIM, X_DIM>& A, Matrix<X_DIM, U_DIM>& B, SymmetricMatrix<X_DIM>& M, unsigned int flag)
{
	if (flag & COMPUTE_c) c = f(xBar, uBar);
	if (flag & COMPUTE_A) A = dfdx(f, xBar, uBar); //identity<X_DIM>();
	if (flag & COMPUTE_B) B = dfdu(f, xBar, uBar); //identity<U_DIM>();
	if (flag & COMPUTE_M) M = varM(xBar, uBar);
}

inline void linearizeObservation(const Matrix<X_DIM>& xBar, Matrix<Z_DIM, X_DIM>& H, SymmetricMatrix<Z_DIM>& N)
{
	H = dhdx(h, xBar); // identity<X_DIM>();
	N = varN(xBar);
}


double costfunc(const std::vector<Matrix<B_DIM> >& B, const std::vector<Matrix<U_DIM> >& U)
{
	double cost = 0;

	Matrix<X_DIM> x;
	Matrix<B_DIM> b;
	Matrix<X_DIM,X_DIM> Sigma;

	b = B[0];

	unVec(b, x, Sigma);
	for (int t = 0; t < T - 1; ++t)
	{
		cost += alpha_belief*tr(Sigma) + alpha_control*tr(~U[t]*U[t]);

		b = beliefDynamics(b, U[t], false);
		//b = B[t+1];
		unVec(b, x, Sigma);
	}
	//unVec(B[T-1], x, Sigma);
	cost += alpha_belief*tr(Sigma);

	return cost;
}

Matrix<X_DIM> SigmaDiag(const Matrix<X_DIM,X_DIM>& Sigma) {
	Matrix<X_DIM> diag;
	for(int i = 0; i < X_DIM; ++i) {
		diag[i] = Sigma(i,i);
	}
	return diag;
}

int main(int argc, char* argv[])
{
	double length1_est = .05,
			length2_est = .05,
			mass1_est = .105,
			mass2_est = .089;

	// position, then velocity
	x0[0] = -M_PI/2.0; x0[1] = -M_PI/2.0; x0[2] = 0; x0[3] = 0;
	// parameter start estimates (alphabetical, then numerical order)
	x0[4] = 1/length1_est; x0[5] = 1/length2_est; x0[6] = 1/mass1_est; x0[7] = 1/mass2_est;

	xGoal[0] = -M_PI/2.0; xGoal[1] = -M_PI/2.0; xGoal[2] = 0.0; xGoal[3] = 0.0;
	xGoal[4] = 1/length1_est; xGoal[5] = 1/length2_est; xGoal[6] = 1/mass1_est; xGoal[7] = 1/mass2_est;


	// init controls from start to goal -- straight line trajectory
	// TODO: possibly switch to random controls
	std::vector< Matrix<U_DIM> > uBar((T-1), (xGoal.subMatrix<U_DIM,1>(0,0) - x0.subMatrix<U_DIM,1>(0,0))/(double)(T-1));

	Rint = 1000.0*identity<U_DIM>();

	Qint(0,0) = 1.0;
	Qint(1,1) = 1.0;
	Qint(2,2) = 1.0;
	Qint(3,3) = 1.0;
	Qint(4,4) = 0.0;
	Qint(5,5) = 0.0;
	Qint(6,6) = 0.0;
	Qint(7,7) = 0.0;

	QintVariance(0,0) = 0.0;
	QintVariance(1,1) = 0.0;
	QintVariance(2,2) = 0.0;
	QintVariance(3,3) = 0.0;
	QintVariance(4,4) = 10000000.0;
	QintVariance(5,5) = 10000000.0;
	QintVariance(6,6) = 1000000.0;
	QintVariance(7,7) = 1000000.0;

	QGoal(0,0) = 1.0;
	QGoal(1,1) = 1.0;
	QGoal(2,2) = 1.0;
	QGoal(3,3) = 1.0;
	QGoal(4,4) = 0.0;
	QGoal(5,5) = 0.0;
	QGoal(6,6) = 0.0;
	QGoal(7,7) = 0.0;

	QGoalVariance(0,0) = 0.0;
	QGoalVariance(1,1) = 0.0;
	QGoalVariance(2,2) = 0.0;
	QGoalVariance(3,3) = 0.0;
	QGoalVariance(4,4) = 10000000.0;
	QGoalVariance(5,5) = 10000000.0;
	QGoalVariance(6,6) = 1000000.0;
	QGoalVariance(7,7) = 1000000.0;

	std::vector< Matrix<U_DIM, X_DIM> > L;
	std::vector< Matrix<X_DIM> > xBar;
	std::vector< SymmetricMatrix<X_DIM> > SigmaBar;
	
	Sigma0(4,4) = 0.5;
	Sigma0(5,5) = 0.5;
	Sigma0(6,6) = 1.0;
	Sigma0(7,7) = 1.0;

	xBar.push_back(x0);
	SigmaBar.push_back(Sigma0);

	std::cout << "solvePOMDP" << std::endl;

	solvePOMDP(linearizeDynamics, linearizeObservation, quadratizeFinalCost, quadratizeCost, xBar, SigmaBar, uBar, L);

	std::cout << "POMDP solved" << std::endl;

	//for (size_t i = 0; i < xBar.size(); ++i) {
	//	std::cout << ~(xBar[i]);
	//}

	std::vector< Matrix<B_DIM> > Bpomdp(T);
	for(int t = 0; t < T; ++t) {
		vec(xBar[t], SigmaBar[t], Bpomdp[t], false);
	}

	std::vector< Matrix<B_DIM> > Bekf(T);
	vec(xBar[0], SigmaBar[0], Bekf[0]);
	for (int t = 0; t < T - 1; ++t)
	{
		Bekf[t+1] = beliefDynamics(Bekf[t], uBar[t], false);
	}

	double cost_pomdp = costfunc(Bpomdp, uBar);
	double cost_ekf = costfunc(Bekf, uBar);

	std::cout << "cost pomdp: " << cost_pomdp << std::endl;
	std::cout << "cost ekf: " << cost_ekf << std::endl;

	Matrix<X_DIM> xpomdp, xekf;
	Matrix<X_DIM,X_DIM> Sigmapomdp, Sigmaekf;
	for(int t = 0; t < T; ++t) {
		unVec(Bpomdp[t], xpomdp, Sigmapomdp);
		unVec(Bekf[t], xekf, Sigmaekf);
		//std::cout << "t: " << t << " xpomdp" << std::endl;
		//std::cout << ~xpomdp;
		//std::cout << "t: " << t << " xekf" << std::endl;
		//std::cout << ~xekf << std::endl;
		std::cout << "t: " << t << " Sigmapomdp" << std::endl;
		std::cout << ~SigmaDiag(Sigmapomdp);
		std::cout << "t: " << t << " Sigmaekf" << std::endl;
		std::cout << ~SigmaDiag(Sigmaekf) << std::endl << std::endl;
	}

#define CPP_PLOT
#ifdef CPP_PLOT

	Py_Initialize();

	py::list Bvec;
	for(int j=0; j < B_DIM; j++) {
		for(int i=0; i < T; i++) {
			Bvec.append(Bpomdp[i][j]);
		}
	}

	py::list Uvec;
	for(int j=0; j < U_DIM; j++) {
		for(int i=0; i < T-1; i++) {
			Uvec.append(uBar[i][j]);
		}
	}

	std::string workingDir = boost::filesystem::current_path().normalize().string();

	py::object main_module = py::import("__main__");
	py::object main_namespace = main_module.attr("__dict__");
	py::exec("import sys, os", main_namespace);
	py::exec(py::str("sys.path.append('"+workingDir+"/parameter')"), main_namespace);
	py::object plot_mod = py::import("plot_parameter");
	py::object plot_traj = plot_mod.attr("plot_parameter_trajectory");

	plot_traj(Bvec, Uvec, B_DIM, X_DIM, U_DIM, T);

	//int k;
	//std::cin >> k;
#endif


	return 0;
}

