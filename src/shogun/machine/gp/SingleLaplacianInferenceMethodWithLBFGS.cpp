/*
 * Copyright (c) The Shogun Machine Learning Toolbox
 * Written (w) 2014 Wu Lin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the Shogun Development Team.
 *
 * Code adapted from Gaussian Process Machine Learning Toolbox
 * http://www.gaussianprocess.org/gpml/code/matlab/doc/
 */
#include <shogun/machine/gp/SingleLaplacianInferenceMethodWithLBFGS.h>

#ifdef HAVE_EIGEN3
#include <shogun/mathematics/Math.h>
#include <shogun/optimization/lbfgs/lbfgs.h>
#include <shogun/mathematics/eigen3.h>

namespace shogun
{

CSingleLaplacianInferenceMethodWithLBFGS::CSingleLaplacianInferenceMethodWithLBFGS()
	: CSingleLaplacianInferenceMethod()
{
	init();
}

CSingleLaplacianInferenceMethodWithLBFGS::CSingleLaplacianInferenceMethodWithLBFGS(
	CKernel* kern,
	CFeatures* feat,
	CMeanFunction* m,
	CLabels* lab,
	CLikelihoodModel* mod)
	: CSingleLaplacianInferenceMethod(kern, feat, m, lab, mod)
{
	init();
}

void CSingleLaplacianInferenceMethodWithLBFGS::set_lbfgs_parameters(
	int m,
	int max_linesearch,
	int linesearch,
	int max_iterations,
	float64_t delta,
	int past,
	float64_t epsilon,
	bool enable_newton_if_fail,
	float64_t min_step,
	float64_t max_step,
	float64_t ftol,
	float64_t wolfe,
	float64_t gtol,
	float64_t xtol,
	float64_t orthantwise_c,
	int orthantwise_start,
	int orthantwise_end)
{
	m_m = m;
	m_max_linesearch = max_linesearch;
	m_linesearch = linesearch;
	m_max_iterations = max_iterations;
	m_delta = delta;
	m_past = past;
	m_epsilon = epsilon;
	m_min_step = min_step;
	m_max_step = max_step;
	m_ftol = ftol;
	m_wolfe = wolfe;
	m_gtol = gtol;
	m_xtol = xtol;
	m_orthantwise_c = orthantwise_c;
	m_orthantwise_start = orthantwise_start;
	m_orthantwise_end = orthantwise_end;
	m_enable_newton_if_fail = enable_newton_if_fail;
}

void CSingleLaplacianInferenceMethodWithLBFGS::init()
{
	set_lbfgs_parameters();
	m_mean_f = NULL;
	SG_ADD(&m_m, "m",
		"The number of corrections to approximate the inverse hessian matrix",
		MS_NOT_AVAILABLE);
	SG_ADD(&m_max_linesearch, "max_linesearch",
		"The maximum number of trials to do line search for each L-BFGS update",
		MS_NOT_AVAILABLE);
	SG_ADD(&m_linesearch, "linesearch",
		"The line search algorithm",
		MS_NOT_AVAILABLE);
	SG_ADD(&m_max_iterations, "max_iterations",
		"The maximum number of iterations for L-BFGS update",
		MS_NOT_AVAILABLE);
	SG_ADD(&m_delta, "delta",
		"Delta for convergence test based on the change of function value",
		MS_NOT_AVAILABLE);
	SG_ADD(&m_past, "past",
		"Distance for delta-based convergence test",
		MS_NOT_AVAILABLE);
	SG_ADD(&m_epsilon, "epsilon",
		"Epsilon for convergence test based on the change of gradient",
		MS_NOT_AVAILABLE);
	SG_ADD(&m_min_step, "min_step",
		"The minimum step of the line search",
		MS_NOT_AVAILABLE);
	SG_ADD(&m_max_step, "max_step",
		"The maximum step of the line search",
		MS_NOT_AVAILABLE);
	SG_ADD(&m_ftol, "ftol",
		"A parameter used in Armijo condition",
		MS_NOT_AVAILABLE);
	SG_ADD(&m_wolfe, "wolfe",
		"A parameter used in curvature condition",
		MS_NOT_AVAILABLE);
	SG_ADD(&m_gtol, "gtol",
		"A parameter used in Morethuente linesearch to control the accuracy",
		MS_NOT_AVAILABLE);
	SG_ADD(&m_xtol, "xtol",
		"The machine precision for floating-point values",
		MS_NOT_AVAILABLE);
	SG_ADD(&m_orthantwise_c, "orthantwise_c",
		"Coeefficient for the L1 norm of variables",
		MS_NOT_AVAILABLE);
	SG_ADD(&m_orthantwise_start, "orthantwise_start",
		"Start index for computing L1 norm of the variables",
		MS_NOT_AVAILABLE);
	SG_ADD(&m_orthantwise_end, "orthantwise_end",
		"End index for computing L1 norm of the variables",
		MS_NOT_AVAILABLE);
	SG_ADD(&m_enable_newton_if_fail, "enable_newton_if_fail",
		"Enable the original Newton method if the L-BFGS method fails",
		MS_NOT_AVAILABLE);
}

CSingleLaplacianInferenceMethodWithLBFGS::~CSingleLaplacianInferenceMethodWithLBFGS()
{
}

float64_t CSingleLaplacianInferenceMethodWithLBFGS::evaluate(
	void *obj,
	const float64_t *alpha,
	float64_t *gradient,
	const int dim,
	const float64_t step)
{
	/* Note that alpha = alpha_pre_iter - step * gradient_pre_iter */

	/* Unfortunately we can not use dynamic_cast to cast the void * pointer to an
	 * object pointer. Therefore, make sure this method is private.  
	 */
	CSingleLaplacianInferenceMethodWithLBFGS * obj_prt
		= static_cast<CSingleLaplacianInferenceMethodWithLBFGS *>(obj);
	float64_t * alpha_cast = const_cast<float64_t *>(alpha);
	float64_t psi = 0.0;
	obj_prt->get_psi_wrt_alpha(alpha_cast, dim, psi);
	obj_prt->get_gradient_wrt_alpha(alpha_cast, gradient, dim);
	return psi;
}

void CSingleLaplacianInferenceMethodWithLBFGS::update_alpha()
{
	float64_t psi_new;
	float64_t psi_def;

	/* get mean vector and create eigen representation of it*/
	SGVector<float64_t> mean_f = m_mean->get_mean_vector(m_features);
	Eigen::Map<Eigen::VectorXd> eigen_mean_f(mean_f.vector, mean_f.vlen);

	/* create eigen representation of kernel matrix*/
	Eigen::Map<Eigen::MatrixXd> eigen_ktrtr(m_ktrtr.matrix,
		m_ktrtr.num_rows,
		m_ktrtr.num_cols);

	/* create shogun and eigen representation of function vector*/
	m_mu = SGVector<float64_t>(mean_f.vlen);
	Eigen::Map<Eigen::VectorXd> eigen_mu(m_mu, m_mu.vlen);

	if (m_alpha.vlen != m_labels->get_num_labels())
	{
		/* set alpha a zero vector*/
		m_alpha = SGVector<float64_t>(m_labels->get_num_labels());
		m_alpha.zero();

		/* f = mean, if length of alpha and length of y doesn't match*/
		eigen_mu = eigen_mean_f;
		psi_new = -SGVector<float64_t>::sum(
			m_model->get_log_probability_f(m_labels, m_mu));
	}
	else
	{
		/* compute f = K * alpha + m*/
		Eigen::Map<Eigen::VectorXd> eigen_alpha(m_alpha.vector, m_alpha.vlen);
		eigen_mu = eigen_ktrtr * (eigen_alpha * CMath::sq(m_scale)) + eigen_mean_f;
		psi_new = eigen_alpha.dot(eigen_mu - eigen_mean_f) / 2.0;
		psi_new -= SGVector<float64_t>::sum(m_model->get_log_probability_f(m_labels, m_mu));

		psi_def = -SGVector<float64_t>::sum(
			m_model->get_log_probability_f(m_labels, mean_f));

		/* if default is better, then use it*/
		if (psi_def < psi_new)
		{
			m_alpha.zero();
			eigen_mu = eigen_mean_f;
			psi_new = psi_def;
		}
	}
	Eigen::Map<Eigen::VectorXd> eigen_alpha(m_alpha.vector, m_alpha.vlen);
	lbfgs_parameter_t lbfgs_param;
	lbfgs_param.m = m_m;
	lbfgs_param.max_linesearch = m_max_linesearch;
	lbfgs_param.linesearch = m_linesearch;
	lbfgs_param.max_iterations = m_max_iterations;
	lbfgs_param.delta = m_delta;
	lbfgs_param.past = m_past;
	lbfgs_param.epsilon = m_epsilon;
	lbfgs_param.min_step = m_min_step;
	lbfgs_param.max_step = m_max_step;
	lbfgs_param.ftol = m_ftol;
	lbfgs_param.wolfe = m_wolfe;
	lbfgs_param.gtol = m_gtol;
	lbfgs_param.xtol = m_xtol;
	lbfgs_param.orthantwise_c = m_orthantwise_c;
	lbfgs_param.orthantwise_start = m_orthantwise_start;
	lbfgs_param.orthantwise_end = m_orthantwise_end;

	/* use for passing variables to compute function value and gradient*/
	m_mean_f = &mean_f;

	/* In order to use the provided lbfgs function, we have to pass the object via
	 * void * pointer, which the evaluate method will use static_cast to cast
	 * the pointer to an object pointer.
	 * Therefore, make sure the evaluate method is a private method of the class. 
	 * Because the evaluate method is defined in a class, we have to pass the
	 * method pointer to the lbfgs function via static method
	 * If we also use the progress method, make sure the method is static and
	 * private. 
	 */
	void * obj_prt = static_cast<void *>(this);

	int ret = lbfgs(m_alpha.vlen, m_alpha.vector, &psi_new,
		CSingleLaplacianInferenceMethodWithLBFGS::evaluate,
		NULL, obj_prt, &lbfgs_param);
	/* clean up*/
	m_mean_f = NULL;

	/* Note that ret should be zero if the minimization 
	 * process terminates without an error.
	 * A non-zero value indicates an error. 
	 */
	if (m_enable_newton_if_fail && ret != 0 && ret != LBFGS_ALREADY_MINIMIZED)
	{
		/* If some error happened during the L-BFGS optimization, we use the original
		 * Newton method.
		 */
		SG_WARNING("Error during L-BFGS optimization, using original Newton method as fallback\n");
		CSingleLaplacianInferenceMethod::update_alpha();
		return;
	}

	/* compute f = K * alpha + m*/
	eigen_mu = eigen_ktrtr * (eigen_alpha * CMath::sq(m_scale)) + eigen_mean_f;

	/* get log probability derivatives*/
	m_dlp  = m_model->get_log_probability_derivative_f(m_labels, m_mu, 1);
	m_d2lp = m_model->get_log_probability_derivative_f(m_labels, m_mu, 2);
	m_d3lp = m_model->get_log_probability_derivative_f(m_labels, m_mu, 3);

	/* W = -d2lp*/
	m_W = m_d2lp.clone();
	m_W.scale(-1.0);

	/* compute sW*/
	Eigen::Map<Eigen::VectorXd> eigen_W(m_W.vector, m_W.vlen);
	/* create shogun and eigen representation of sW*/
	m_sW = SGVector<float64_t>(m_W.vlen);
	Eigen::Map<Eigen::VectorXd> eigen_sW(m_sW.vector, m_sW.vlen);

	if (eigen_W.minCoeff() > 0)
		eigen_sW = eigen_W.cwiseSqrt();
	else
		eigen_sW.setZero();
}

void CSingleLaplacianInferenceMethodWithLBFGS::get_psi_wrt_alpha(
	float64_t *alpha,
	const int dim,
	float64_t &psi)
{
	Eigen::Map<Eigen::VectorXd> eigen_alpha(alpha, dim);
	SGVector<float64_t> f(dim);
	Eigen::Map<Eigen::VectorXd> eigen_f(f.vector, f.vlen);
	Eigen::Map<Eigen::MatrixXd> kernel(m_ktrtr.matrix,
		m_ktrtr.num_rows,
		m_ktrtr.num_cols);
	Eigen::Map<Eigen::VectorXd> eigen_mean_f(m_mean_f->vector,
		m_mean_f->vlen);
	/* f = K * alpha + mean_f given alpha*/
	eigen_f
		= kernel * ((eigen_alpha) * CMath::sq(m_scale)) + eigen_mean_f;

	/* psi = 0.5 * alpha .* (f - m) - sum(dlp)*/
	psi = eigen_alpha.dot(eigen_f - eigen_mean_f) * 0.5;
	psi -= SGVector<float64_t>::sum(m_model->get_log_probability_f(m_labels, f));
}

void CSingleLaplacianInferenceMethodWithLBFGS::get_gradient_wrt_alpha(
	float64_t *alpha,
	float64_t *gradient,
	const int dim)
{
	Eigen::Map<Eigen::VectorXd> eigen_alpha(alpha, dim);
	Eigen::Map<Eigen::VectorXd> eigen_gradient(gradient, dim);
	SGVector<float64_t> f(dim);
	Eigen::Map<Eigen::VectorXd> eigen_f(f.vector, f.vlen);
	Eigen::Map<Eigen::MatrixXd> kernel(m_ktrtr.matrix,
		m_ktrtr.num_rows,
		m_ktrtr.num_cols);
	Eigen::Map<Eigen::VectorXd> eigen_mean_f(m_mean_f->vector,
		m_mean_f->vlen);

	/* f = K * alpha + mean_f given alpha*/
	eigen_f = kernel * ((eigen_alpha) * CMath::sq(m_scale)) + eigen_mean_f;

	SGVector<float64_t> dlp_f =
		m_model->get_log_probability_derivative_f(m_labels, f, 1);

	Eigen::Map<Eigen::VectorXd> eigen_dlp_f(dlp_f.vector, dlp_f.vlen);

	/* g_alpha = K * (alpha - dlp_f)*/
	eigen_gradient = kernel * ((eigen_alpha - eigen_dlp_f) * CMath::sq(m_scale));
}

} /* namespace shogun */
#endif /* HAVE_EIGEN3 */
