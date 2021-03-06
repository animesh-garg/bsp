/*
FORCES - Fast interior point code generation for multistage problems.
Copyright (C) 2011-14 Alexander Domahidi [domahidi@control.ee.ethz.ch],
Automatic Control Laboratory, ETH Zurich.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __beliefPenaltyMPC_H__
#define __beliefPenaltyMPC_H__


/* DATA TYPE ------------------------------------------------------------*/
typedef double beliefPenaltyMPC_FLOAT;


/* SOLVER SETTINGS ------------------------------------------------------*/
/* print level */
#ifndef beliefPenaltyMPC_SET_PRINTLEVEL
#define beliefPenaltyMPC_SET_PRINTLEVEL    (0)
#endif

/* timing */
#ifndef beliefPenaltyMPC_SET_TIMING
#define beliefPenaltyMPC_SET_TIMING    (0)
#endif

/* Numeric Warnings */
/* #define PRINTNUMERICALWARNINGS */

/* maximum number of iterations  */
#define beliefPenaltyMPC_SET_MAXIT         (30)	

/* scaling factor of line search (affine direction) */
#define beliefPenaltyMPC_SET_LS_SCALE_AFF  (0.9)      

/* scaling factor of line search (combined direction) */
#define beliefPenaltyMPC_SET_LS_SCALE      (0.95)  

/* minimum required step size in each iteration */
#define beliefPenaltyMPC_SET_LS_MINSTEP    (1E-08)

/* maximum step size (combined direction) */
#define beliefPenaltyMPC_SET_LS_MAXSTEP    (0.995)

/* desired relative duality gap */
#define beliefPenaltyMPC_SET_ACC_RDGAP     (0.0001)

/* desired maximum residual on equality constraints */
#define beliefPenaltyMPC_SET_ACC_RESEQ     (1E-06)

/* desired maximum residual on inequality constraints */
#define beliefPenaltyMPC_SET_ACC_RESINEQ   (1E-06)

/* desired maximum violation of complementarity */
#define beliefPenaltyMPC_SET_ACC_KKTCOMPL  (1E-06)


/* RETURN CODES----------------------------------------------------------*/
/* solver has converged within desired accuracy */
#define beliefPenaltyMPC_OPTIMAL      (1)

/* maximum number of iterations has been reached */
#define beliefPenaltyMPC_MAXITREACHED (0)

/* no progress in line search possible */
#define beliefPenaltyMPC_NOPROGRESS   (-7)




/* PARAMETERS -----------------------------------------------------------*/
/* fill this with data before calling the solver! */
typedef struct beliefPenaltyMPC_params
{
    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q1[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f1[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb1[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub1[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C1[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e1[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q2[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f2[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb2[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub2[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C2[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e2[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q3[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f3[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb3[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub3[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C3[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e3[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q4[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f4[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb4[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub4[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C4[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e4[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q5[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f5[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb5[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub5[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C5[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e5[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q6[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f6[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb6[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub6[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C6[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e6[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q7[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f7[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb7[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub7[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C7[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e7[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q8[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f8[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb8[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub8[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C8[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e8[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q9[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f9[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb9[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub9[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C9[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e9[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q10[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f10[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb10[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub10[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C10[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e10[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q11[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f11[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb11[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub11[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C11[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e11[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q12[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f12[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb12[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub12[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C12[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e12[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q13[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f13[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb13[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub13[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C13[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e13[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q14[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f14[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb14[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub14[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C14[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e14[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q15[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f15[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb15[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub15[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C15[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e15[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q16[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f16[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb16[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub16[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C16[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e16[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q17[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f17[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb17[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub17[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C17[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e17[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q18[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f18[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb18[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub18[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C18[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e18[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q19[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f19[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb19[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub19[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C19[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e19[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q20[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f20[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb20[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub20[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C20[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e20[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q21[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f21[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb21[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub21[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C21[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e21[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q22[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f22[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb22[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub22[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C22[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e22[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q23[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f23[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb23[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub23[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C23[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e23[44];

    /* diagonal matrix of size [134 x 134] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q24[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT f24[134];

    /* vector of size 134 */
    beliefPenaltyMPC_FLOAT lb24[134];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT ub24[46];

    /* matrix of size [44 x 134] (column major format) */
    beliefPenaltyMPC_FLOAT C24[5896];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e24[44];

    /* diagonal matrix of size [44 x 44] (only the diagonal is stored) */
    beliefPenaltyMPC_FLOAT Q25[44];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT f25[44];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT lb25[44];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT ub25[44];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT e25[44];

    /* vector of size 16 */
    beliefPenaltyMPC_FLOAT b25[16];

} beliefPenaltyMPC_params;


/* OUTPUTS --------------------------------------------------------------*/
/* the desired variables are put here by the solver */
typedef struct beliefPenaltyMPC_output
{
    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z1[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z2[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z3[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z4[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z5[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z6[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z7[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z8[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z9[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z10[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z11[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z12[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z13[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z14[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z15[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z16[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z17[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z18[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z19[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z20[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z21[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z22[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z23[46];

    /* vector of size 46 */
    beliefPenaltyMPC_FLOAT z24[46];

    /* vector of size 44 */
    beliefPenaltyMPC_FLOAT z25[44];

} beliefPenaltyMPC_output;


/* SOLVER INFO ----------------------------------------------------------*/
/* diagnostic data from last interior point step */
typedef struct beliefPenaltyMPC_info
{
    /* iteration number */
    int it;
	
    /* inf-norm of equality constraint residuals */
    beliefPenaltyMPC_FLOAT res_eq;
	
    /* inf-norm of inequality constraint residuals */
    beliefPenaltyMPC_FLOAT res_ineq;

    /* primal objective */
    beliefPenaltyMPC_FLOAT pobj;	
	
    /* dual objective */
    beliefPenaltyMPC_FLOAT dobj;	

    /* duality gap := pobj - dobj */
    beliefPenaltyMPC_FLOAT dgap;		
	
    /* relative duality gap := |dgap / pobj | */
    beliefPenaltyMPC_FLOAT rdgap;		

    /* duality measure */
    beliefPenaltyMPC_FLOAT mu;

	/* duality measure (after affine step) */
    beliefPenaltyMPC_FLOAT mu_aff;
	
    /* centering parameter */
    beliefPenaltyMPC_FLOAT sigma;
	
    /* number of backtracking line search steps (affine direction) */
    int lsit_aff;
    
    /* number of backtracking line search steps (combined direction) */
    int lsit_cc;
    
    /* step size (affine direction) */
    beliefPenaltyMPC_FLOAT step_aff;
    
    /* step size (combined direction) */
    beliefPenaltyMPC_FLOAT step_cc;    

	/* solvertime */
	beliefPenaltyMPC_FLOAT solvetime;   

} beliefPenaltyMPC_info;


/* SOLVER FUNCTION DEFINITION -------------------------------------------*/
/* examine exitflag before using the result! */
int beliefPenaltyMPC_solve(beliefPenaltyMPC_params* params, beliefPenaltyMPC_output* output, beliefPenaltyMPC_info* info);


#endif