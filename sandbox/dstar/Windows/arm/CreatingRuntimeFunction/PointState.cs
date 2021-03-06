using System;
using System.Collections.Generic;
using System.Linq;
using System.Linq.Expressions;
using System.Text;
using System.Diagnostics;
using DifferentiableFunction;
using matrix;
using utilities;
//using lapack;
using System.IO;

// Figure out how to pass in matrix arguments to a function and evaluate a runtime function -- DONE
// -- order variables to pass in to function? -- SORT OF
// test ekf implementation -- DONE
// belief dynamics function compile -- DONE
// cost function based on belief eval function compile -- DONE
// cleanup code -- DONE
// derivative of the cost function w.r.t state and control inputs -- Jacobian and Hessian -- DONE

namespace Example_CreatingRuntimeFunction
{
    public class PointState : Point
    {
        
        public PointState(int timesteps) : base(timesteps) { }
        

       
        Function costfunc(Function[][] X, Function[][] U, Function[] q, Function[] r, Function[,] Sigma_0, Function alpha_belief, Function alpha_control, Function alpha_final_belief)
        {
            int T = X.GetLength(0);

            Function cost = Constant.newConstant(0.0);
            
            Function[] x_tp1 = X[0];
            Function[,] Sigma_t = Sigma_0, Sigma_tp1;

            for (int t = 0; t < T - 1; ++t)
            {
                cost = cost + alpha_belief * VM.trace(Sigma_t);
                cost = cost + alpha_control * VM.dot(U[t], U[t]);

                EKF(X[t], U[t], q, r, Sigma_t, out x_tp1, out Sigma_tp1);
                Sigma_t = Sigma_tp1;
            }

            cost = cost + alpha_final_belief * VM.trace(Sigma_t);
            return cost;
        }


        int initVars(Variable[] vars)
        {
            X = new Function[T][];
            U = new Function[T - 1][];

            int idx = 0;
            for (int t = 0; t < T; ++t)
            {
                X[t] = new Function[XDIM];
                for (int i = 0; i < XDIM; ++i)
                {
                    X[t][i] = vars[idx++];
                }
            }
            for (int t = 0; t < (T - 1); ++t)
            {
                U[t] = new Function[UDIM];
                for (int i = 0; i < UDIM; ++i)
                {
                    U[t][i] = vars[idx++];
                }
            }
            q = new Function[] { vars[idx++], vars[idx++] };
            r = new Function[] { vars[idx++], vars[idx++] };

            Sigma_0 = new Function[XDIM, XDIM];
            for (int i = 0; i < XDIM; ++i)
            {
                for (int j = 0; j < XDIM; ++j)
                {
                    Sigma_0[i, j] = vars[idx++];
                }
            }
            return idx;
        }
        

        void computeCostGradDiagHess(string eval_name)
        {
            uint flag = 0;
            flag |= ((uint)(Convert.ToUInt16(eval_name.Contains("Cost"))) << 0);
            flag |= ((uint)(Convert.ToUInt16(eval_name.Contains("Grad"))) << 1);
            flag |= ((uint)(Convert.ToUInt16(eval_name.Contains("DiagHess"))) << 3);
            
            // variable instantiations
            int nparams = 3;
            int nvars = T * XDIM + (T - 1) * UDIM + QDIM + RDIM + (XDIM * XDIM) + nparams;

            Variable[] vars = new Variable[nvars];
            for (int i = 0; i < nvars; ++i) { vars[i] = new Variable("vars_" + i); }

            int idx = initVars(vars);

            Function alpha_belief = vars[idx++];
            Function alpha_control = vars[idx++];
            Function alpha_final_belief = vars[idx++];
 
            Function[] XU = VM.concatenate<Function>(VM.jaggedToLinear<Function>(X), VM.jaggedToLinear<Function>(U));

            Function cost = costfunc(X, U, q, r, Sigma_0, alpha_belief, alpha_control, alpha_final_belief);

            List<Function> costJacDiagHessFuncList = new List<Function>();
            if ((flag & COMPUTE_COST) != 0)
            {
                costJacDiagHessFuncList.Add(cost);
            }
            
            if ((flag & COMPUTE_JACOBIAN) != 0)
            {
	            for (int i = 0; i < XU.Length; ++i)
	            {
	                costJacDiagHessFuncList.Add(Function.D(cost, XU[i]));
	            }
            }
            
            if ((flag & COMPUTE_DIAGONAL_HESSIAN) != 0)
            {
	            for (int i = 0; i < XU.Length; ++i)
	            {
	                costJacDiagHessFuncList.Add(Function.D(cost, XU[i], XU[i]));
	            }
            }
            
            Function costJacDiagHess = Function.derivative(costJacDiagHessFuncList.ToArray());
            costJacDiagHess.printOperatorCounts();

            bool[] inputVarIndices;
            Variable[] costJacDiagHessVars = initializeInputVariables(costJacDiagHess, vars, out inputVarIndices);
            costJacDiagHess.orderVariablesInDomain(costJacDiagHessVars);

            costJacDiagHess.compileCCodeToFile("state-symeval-"+T+".c");

            string fileName = "state-masks-"+T+".txt";
            Console.WriteLine("Writing " + fileName);
            System.IO.StreamWriter fh = new System.IO.StreamWriter(fileName);
            for (int i = 0; i < nvars; ++i)
            {
                if (inputVarIndices[i])
                {
                    fh.Write("1 ");
                }
                else
                {
                    fh.Write("0 ");
                }
            }
            fh.WriteLine();
            fh.Close();        
        }


        /*
        static void Main(string[] args)
        { 
            // T = args[0]
            int T = 15;
            string eval_name = "CostGradDiagHess"; // default to comput cost, gradient, and diagonal hessian
           
            if (args.Length >= 1) {
                T = int.Parse(args[0]);
            }
            if (args.Length >= 2) {
                eval_name = args[1];
            }
           
            Console.WriteLine("Creating files for "+eval_name+" for T = "+T);
           
            Function.newContext();
            Function.printCompilerSource = false;

            PointState ps = new PointState(T);

            var stopwatch = new Stopwatch();
            stopwatch.Start();

            ps.computeCostGradDiagHess(eval_name);

           
            stopwatch.Stop();
            
            Console.WriteLine("Finished in " + (stopwatch.ElapsedMilliseconds/1000.0) + " s");
        }
         */
    }
}