#include <vector>
#include <iomanip>


#include "../parameter.h"

#include "util/matrix.h"
//#include "util/Timer.h"
#include "util/logging.h"
//#include "util/utils.h"

#include <Python.h>
#include <boost/python.hpp>


#include <boost/random.hpp>
#include <boost/random/normal_distribution.hpp>

namespace py = boost::python;

Matrix<X_DIM> x0;
Matrix<X_DIM,X_DIM> SqrtSigma0;
Matrix<X_DIM> xGoal;
Matrix<X_DIM> xMin, xMax;
Matrix<U_DIM> uMin, uMax;

boost::mt19937 rng; 
boost::uniform_real<> dist(-0.1, 0.1);

// utility to fill Matrix in column major format in FORCES array
template <size_t _numRows>
inline void fillCol(double *X, const Matrix<_numRows>& XCol) {
	int idx = 0;
	for(size_t r = 0; r < _numRows; ++r) {
		X[idx++] = XCol[r];
	}
}

// utility to fill Matrix in column major format in FORCES array

template <size_t _numRows, size_t _numColumns>
inline void fillColMajor(double *X, const Matrix<_numRows, _numColumns>& XMat) {
	int idx = 0;
	for(size_t c = 0; c < _numColumns; ++c) {
		for(size_t r = 0; r < _numRows; ++r) {
			X[idx++] = XMat[c + r*_numColumns];
		}
	}
}


double uRand(){

	
  	boost::variate_generator<boost::mt19937&, 
                           boost::uniform_real<> > random(rng, dist);
	
	return random();
}



int main(int argc, char* argv[])
{

	// actual: 0.15, 0.15, 0.1, 0.1
	double length1_est = .3,
			length2_est = .7,
			mass1_est = .3,
			mass2_est = .35;

	// position, then velocity
	x0[0] = M_PI*0.5; x0[1] = M_PI*0.5; x0[2] = 0; x0[3] = 0;
	// parameter start estimates (alphabetical, then numerical order)
	x0[4] = 1/length1_est; x0[5] = 1/length2_est; x0[6] = 1/mass1_est; x0[7] = 1/mass2_est;


	Matrix<X_DIM> x_real;
	x_real[0] = M_PI*0.45; x_real[1] = M_PI*0.55; x_real[2] = -0.01; x_real[3] = 0.01;
	x_real[4] = 1/dynamics::length1; x_real[5] = 1/dynamics::length2; x_real[6] = 1/dynamics::mass1; x_real[7] = 1/dynamics::mass2;



	// from original file, possibly change
	SqrtSigma0(0,0) = 0.1;
	SqrtSigma0(1,1) = 0.1;
	SqrtSigma0(2,2) = 0.05;
	SqrtSigma0(3,3) = 0.05;
	SqrtSigma0(4,4) = 0.5;
	SqrtSigma0(5,5) = 0.5;
	SqrtSigma0(6,6) = 0.5;
	SqrtSigma0(7,7) = 0.5;
	//Matrix<U_DIM> uinit = (xGoal.subMatrix<U_DIM,1>(0,0) - x0.subMatrix<U_DIM,1>(0,0))/(double)(T-1);
	Matrix<U_DIM> uinit;
	uinit[0] = -.07;
	uinit[1] = .07;
	
	std::vector<Matrix<U_DIM> > U(T-1, uinit); 
	std::vector<Matrix<B_DIM> > B(T);

	std::vector<Matrix<U_DIM> > HistoryU(HORIZON);
	std::vector<Matrix<B_DIM> > HistoryB(HORIZON);

	vec(x0, SqrtSigma0, B[0]);

	for(int h = 0; h < HORIZON; ++h) {

		for(int t = 0; t < T-1; ++t) {
			for(int i = 0; i<U_DIM; i++){
				U[t][i] = -U[t][i];
			}
		}

		for (int t = 0; t < T-1; ++t) {
			B[t+1] = beliefDynamics(B[t], U[t]);
		}

		//util::Timer solveTimer;
		//util::Timer_tic(&solveTimer);

		
	

		//pythonDisplayTrajectory(U, SqrtSigma0, x0, xGoal);
		//pythonPlotRobot(U, SqrtSigma0, x0, xGoal);

		//double solvetime = util::Timer_toc(&solveTimer);
		//LOG_INFO("Optimized cost: %4.10f", cost);
		//LOG_INFO("Solve time: %5.3f ms", solvetime*1000);
		
		unVec(B[0], x0, SqrtSigma0);

		//std::cout << "x0 before control step" << std::endl << ~x0;
		//std::cout << "control u: " << std::endl << ~U[0];

		//pythonDisplayTrajectory(U, SqrtSigma0, x0, xGoal);
		
		LOG_INFO("Executing control step");
		
		HistoryU[h] = U[0];
		HistoryB[h] = B[0];
		
		
		B[0] = executeControlStep(x_real, B[0], U[0]);
		

		std::cout<<U[0]<<"\n";

		unVec(B[0], x0, SqrtSigma0);
		//std::cout << "x0 after control step" << std::endl << ~x0;

		
	}
	std::cout<<"done\n";

	


#define PLOT
#ifdef PLOT
	pythonDisplayHistory(HistoryU,HistoryB, SqrtSigma0, x0, HORIZON);

	//int k;
	//std::cin >> k;
#endif

	return 0;
}
