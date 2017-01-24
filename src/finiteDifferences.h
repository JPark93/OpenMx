#ifndef _finiteDifferences_H_
#define _finiteDifferences_H_

#include <limits>

// See http://en.wikipedia.org/wiki/Finite_difference

template <class Derived>
struct finite_difference_grad {
	double refFit;
	int thrId;
	double *point;
	double orig;

	template <typename T1>
	double approx(T1 ff, double offset, int px)
	{
		return static_cast<Derived *>(this)->approx(ff, offset, px);
	}

	template <typename T1>
	void operator()(T1 ff, double _refFit, int _thrId, double *_point,
			double offset, int px, int numIter, double *Gaprox)
	{
		refFit = _refFit;
		thrId = _thrId;
		point = _point;
		orig = point[px];

		for(int k = 0; k < numIter;) {
			double grad = 0;
			if (offset > std::numeric_limits<double>::epsilon()) {
				grad = approx(ff, offset, px);
			}
			offset *= .5;
			if (!std::isfinite(grad)) {
				if (OMX_DEBUG) mxLog("finite differences[%d]: retry with offset %.4g",
						     px, offset);
				continue;
			}
			Gaprox[k] = grad;
			k += 1;
		}
		point[px] = orig;
	};
};

struct forward_difference_grad : finite_difference_grad<forward_difference_grad> {
	template <typename T1>
	double approx(T1 ff, double offset, int px)
	{
		point[px] = orig + offset;
		double f1 = ff(point, thrId);
		return (f1 - refFit) / offset;
	}
};

struct central_difference_grad  : finite_difference_grad<central_difference_grad> {
	template <typename T1>
	double approx(T1 ff, double offset, int px)
	{
		point[px] = orig + offset;
		double f1 = ff(point, thrId);
		point[px] = orig - offset;
		double f2 = ff(point, thrId);
		return (f1 - f2) / (2.0 * offset);
	}
};

template <typename T1, typename T2, typename T3, typename T4>
void gradientImpl(T1 ff, int numThreads, double refFit, Eigen::MatrixBase<T2> &point, int numIter,
		  const double eps, T4 dfn, Eigen::MatrixBase<T3> &gradOut)
{
	Eigen::MatrixXd grid(numIter, point.size());
	Eigen::MatrixXd thrPoint(point.size(), numThreads);
	thrPoint.colwise() = point;

#pragma omp parallel for num_threads(numThreads)
	for (int px=0; px < int(point.size()); ++px) {
		int thrId = omp_get_thread_num();
		int thrSelect = numThreads==1? -1 : thrId;
		double offset = std::max(fabs(point[px] * eps), eps);
		dfn[thrId](ff, refFit, thrSelect, &thrPoint.coeffRef(0, thrId), offset, px, numIter, &grid.coeffRef(0,px));
		// push down TODO
		for(int m = 1; m < numIter; m++) {	// Richardson Step
			for(int k = 0; k < (numIter - m); k++) {
				// NumDeriv Hard-wires 4s for r here. Why?
				grid(k,px) = (grid(k+1,px) * pow(4.0, m) - grid(k,px))/(pow(4.0, m)-1);
			}
		}
		gradOut[px] = grid(0,px);
	}
}

template <typename T1, typename T2, typename T3>
void gradient_with_ref(GradientAlgorithm algo, int numThreads, int order, double eps, T1 ff, double refFit,
		       Eigen::MatrixBase<T2> &point, Eigen::MatrixBase<T3> &gradOut)
{
	numThreads = std::min(numThreads, point.size()); // could break work into smaller pieces TODO
	switch (algo) {
	case GradientAlgorithm_Forward:{
		std::vector<forward_difference_grad> dfn(numThreads);
		gradientImpl(ff, numThreads, refFit, point, order, eps, dfn, gradOut);
		break;}
	case GradientAlgorithm_Central:{
		std::vector<central_difference_grad> dfn(numThreads);
		gradientImpl(ff, numThreads, refFit, point, order, eps, dfn, gradOut);
		break;}
	default: Rf_error("Unknown gradient algorithm %d", algo);
	}
}

struct forward_difference_jacobi {
	template <typename T1, typename T2, typename T3, typename T4>
	void operator()(T1 ff, Eigen::MatrixBase<T4> &refFit, Eigen::MatrixBase<T2> &point,
			double offset, int px, int numIter, Eigen::MatrixBase<T3> &Gaprox)
	{
		double orig = point[px];
		Eigen::VectorXd result(refFit.size());
		for(int k = 0; k < numIter; k++) {
			point[px] = orig + offset;
			ff(point, result);
			Gaprox.col(k) = (result - refFit) / offset;
			offset *= .5;
		}
		point[px] = orig;
	};
};

struct central_difference_jacobi {
	template <typename T1, typename T2, typename T3, typename T4>
	void operator()(T1 ff, Eigen::MatrixBase<T4> &refFit, Eigen::MatrixBase<T2> &point,
			double offset, int px, int numIter, Eigen::MatrixBase<T3> &Gaprox)
	{
		double orig = point[px];
		Eigen::VectorXd result1(refFit.size());
		Eigen::VectorXd result2(refFit.size());
		for(int k = 0; k < numIter; k++) {
			point[px] = orig + offset;
			ff(point, result1);
			point[px] = orig - offset;
			ff(point, result2);
			Gaprox.col(k) = (result1 - result2) / (2.0 * offset);
			offset *= .5;
		}
		point[px] = orig;
	};
};

template <typename T1, typename T2, typename T3, typename T4, typename T5>
void jacobianImpl(T1 ff,  Eigen::MatrixBase<T2> &ref, Eigen::MatrixBase<T3> &point,
		  int numIter, const double eps, T4 dfn, Eigen::MatrixBase<T5> &jacobiOut)
{
	// TODO evaluate jacobian in parallel
	for (int px=0; px < int(point.size()); ++px) {
		double offset = std::max(fabs(point[px] * eps), eps);
		Eigen::MatrixXd Gaprox(ref.size(), numIter);
		dfn(ff, ref, point, offset, px, numIter, Gaprox);
		for(int m = 1; m < numIter; m++) {						// Richardson Step
			for(int k = 0; k < (numIter - m); k++) {
				// NumDeriv Hard-wires 4s for r here. Why?
				Gaprox.col(k) = (Gaprox.col(k+1) * pow(4.0, m) - Gaprox.col(k))/(pow(4.0, m)-1);
			}
		}
		jacobiOut.row(px) = Gaprox.col(0).transpose();
	}
}

template <typename T1, typename T2, typename T3, typename T4>
void fd_jacobian(GradientAlgorithm algo, int numIter, double eps, T1 ff, Eigen::MatrixBase<T2> &ref,
	      Eigen::MatrixBase<T3> &point, Eigen::MatrixBase<T4> &jacobiOut)
{
	switch (algo) {
	case GradientAlgorithm_Forward:{
		forward_difference_jacobi dfn;
		jacobianImpl(ff, ref, point, numIter, eps, dfn, jacobiOut);
		break;}
	case GradientAlgorithm_Central:{
		central_difference_jacobi dfn;
		jacobianImpl(ff, ref, point, numIter, eps, dfn, jacobiOut);
		break;}
	default: Rf_error("Unknown gradient algorithm %d", algo);
	}
}

#endif
