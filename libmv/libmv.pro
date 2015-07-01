#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = LibMV
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

include(../global.pri)
include(../config.pri)

INCLUDEPATH += $$PWD/../libs/libmv/third_party/ceres/config
INCLUDEPATH += $$PWD/../libs/libmv/third_party/ceres/include
INCLUDEPATH += $$PWD/../libs/libmv/third_party/ceres/internal
INCLUDEPATH += $$PWD/../libs/libmv/third_party/gflags
INCLUDEPATH += $$PWD/../libs/libmv/third_party
INCLUDEPATH += $$PWD/../libs/libmv/third_party/Eigen3

DEFINES += CERES_HAVE_PTHREAD CERES_NO_SUITESPARSE CERES_NO_CXSPARSE CERES_NO_LAPACK CERES_HAVE_RWLOCK

win32* {
     INCLUDEPATH += $$PWD/../libs/libmv/third_party/glog/src/windows
}

!win32* {
    INCLUDEPATH += $$PWD/../libs/libmv/third_party/glog/src
}

win32-msvc* {
	CONFIG(64bit) {
		QMAKE_LFLAGS += /MACHINE:X64
	} else {
		QMAKE_LFLAGS += /MACHINE:X86
	}
        INCLUDEPATH += $$PWD/../libs/libmv/third_party/msinttypes
}

c++11 {
    DEFINES += CERES_STD_UNORDERED_MAP
} else {
    DEFINES += CERES_NO_UNORDERED_MAP
}


SOURCES += \
         ../libs/libmv/third_party/ceres/internal/ceres/array_utils.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/blas.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/block_evaluate_preparer.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/block_jacobian_writer.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/block_jacobi_preconditioner.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/block_random_access_dense_matrix.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/block_random_access_diagonal_matrix.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/block_random_access_matrix.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/block_random_access_sparse_matrix.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/block_sparse_matrix.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/block_structure.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/callbacks.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/canonical_views_clustering.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/c_api.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/cgnr_solver.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/compressed_col_sparse_matrix_utils.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/compressed_row_jacobian_writer.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/compressed_row_sparse_matrix.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/conditioned_cost_function.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/conjugate_gradients_solver.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/coordinate_descent_minimizer.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/corrector.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/covariance.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/covariance_impl.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/cxsparse.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/dense_normal_cholesky_solver.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/dense_qr_solver.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/dense_sparse_matrix.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/detect_structure.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/dogleg_strategy.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/dynamic_compressed_row_jacobian_writer.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/dynamic_compressed_row_sparse_matrix.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/evaluator.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/file.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_d_d_d.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_d_d_d.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/gradient_checking_cost_function.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/gradient_problem.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/gradient_problem_solver.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/implicit_schur_complement.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/incomplete_lq_factorization.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/iterative_schur_complement_solver.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/lapack.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/levenberg_marquardt_strategy.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/linear_least_squares_problems.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/linear_operator.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/linear_solver.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/line_search.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/line_search_direction.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/line_search_minimizer.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/line_search_preprocessor.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/local_parameterization.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/loss_function.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/low_rank_inverse_hessian.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/minimizer.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/normal_prior.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/parameter_block_ordering.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/partitioned_matrix_view.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/polynomial.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/preconditioner.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/preprocessor.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/problem.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/problem_impl.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/program.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/reorder_program.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/residual_block.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/residual_block_utils.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/schur_complement_solver.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/schur_eliminator.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/schur_jacobi_preconditioner.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/scratch_evaluate_preparer.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/single_linkage_clustering.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/solver.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/solver_utils.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/sparse_matrix.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/sparse_normal_cholesky_solver.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/split.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/stringprintf.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/suitesparse.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/triplet_sparse_matrix.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/trust_region_minimizer.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/trust_region_preprocessor.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/trust_region_strategy.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/types.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/visibility_based_preconditioner.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/visibility.cc \
        ../libs/libmv/third_party/ceres/internal/ceres/wall_time.cc

#Add these source files if doing bundle adjustements, these are template specializations for common cases (6-parameter cameras and 3-parameter points)
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_2_2_2.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_2_2_3.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_2_2_4.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_2_2_d.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_2_3_3.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_2_3_4.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_2_3_9.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_2_3_d.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_2_4_3.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_2_4_4.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_2_4_8.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_2_4_9.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_2_4_d.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_2_d_d.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_4_4_2.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_4_4_3.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_4_4_4.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/partitioned_matrix_view_4_4_d.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_2_2_2.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_2_2_3.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_2_2_4.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_2_2_d.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_2_3_3.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_2_3_4.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_2_3_9.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_2_3_d.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_2_4_3.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_2_4_4.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_2_4_8.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_2_4_9.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_2_4_d.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_2_d_d.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_4_4_2.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_4_4_3.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_4_4_4.cc \
#                ../libs/libmv/third_party/ceres/internal/ceres/generated/schur_eliminator_4_4_d.cc

HEADERS += \
        ../libs/libmv/third_party/ceres/include/ceres/autodiff_cost_function.h \
        ../libs/libmv/third_party/ceres/include/ceres/autodiff_local_parameterization.h \
        ../libs/libmv/third_party/ceres/include/ceres/c_api.h \
        ../libs/libmv/third_party/ceres/include/ceres/ceres.h \
        ../libs/libmv/third_party/ceres/include/ceres/conditioned_cost_function.h \
        ../libs/libmv/third_party/ceres/include/ceres/cost_function.h \
        ../libs/libmv/third_party/ceres/include/ceres/cost_function_to_functor.h \
        ../libs/libmv/third_party/ceres/include/ceres/covariance.h \
        ../libs/libmv/third_party/ceres/include/ceres/crs_matrix.h \
        ../libs/libmv/third_party/ceres/include/ceres/dynamic_autodiff_cost_function.h \
        ../libs/libmv/third_party/ceres/include/ceres/dynamic_numeric_diff_cost_function.h \
        ../libs/libmv/third_party/ceres/include/ceres/fpclassify.h \
        ../libs/libmv/third_party/ceres/include/ceres/gradient_checker.h \
        ../libs/libmv/third_party/ceres/include/ceres/gradient_problem.h \
        ../libs/libmv/third_party/ceres/include/ceres/gradient_problem_solver.h \
        ../libs/libmv/third_party/ceres/include/ceres/internal/autodiff.h \
        ../libs/libmv/third_party/ceres/include/ceres/internal/disable_warnings.h \
        ../libs/libmv/third_party/ceres/include/ceres/internal/eigen.h \
        ../libs/libmv/third_party/ceres/include/ceres/internal/fixed_array.h \
        ../libs/libmv/third_party/ceres/include/ceres/internal/macros.h \
        ../libs/libmv/third_party/ceres/include/ceres/internal/manual_constructor.h \
        ../libs/libmv/third_party/ceres/include/ceres/internal/numeric_diff.h \
        ../libs/libmv/third_party/ceres/include/ceres/internal/port.h \
        ../libs/libmv/third_party/ceres/include/ceres/internal/reenable_warnings.h \
        ../libs/libmv/third_party/ceres/include/ceres/internal/scoped_ptr.h \
        ../libs/libmv/third_party/ceres/include/ceres/internal/variadic_evaluate.h \
        ../libs/libmv/third_party/ceres/include/ceres/iteration_callback.h \
        ../libs/libmv/third_party/ceres/include/ceres/jet.h \
        ../libs/libmv/third_party/ceres/include/ceres/local_parameterization.h \
        ../libs/libmv/third_party/ceres/include/ceres/loss_function.h \
        ../libs/libmv/third_party/ceres/include/ceres/normal_prior.h \
        ../libs/libmv/third_party/ceres/include/ceres/numeric_diff_cost_function.h \
        ../libs/libmv/third_party/ceres/include/ceres/ordered_groups.h \
        ../libs/libmv/third_party/ceres/include/ceres/problem.h \
        ../libs/libmv/third_party/ceres/include/ceres/rotation.h \
        ../libs/libmv/third_party/ceres/include/ceres/sized_cost_function.h \
        ../libs/libmv/third_party/ceres/include/ceres/solver.h \
        ../libs/libmv/third_party/ceres/include/ceres/types.h \
        ../libs/libmv/third_party/ceres/include/ceres/version.h \
        ../libs/libmv/third_party/ceres/internal/ceres/array_utils.h \
        ../libs/libmv/third_party/ceres/internal/ceres/blas.h \
        ../libs/libmv/third_party/ceres/internal/ceres/block_evaluate_preparer.h \
        ../libs/libmv/third_party/ceres/internal/ceres/block_jacobian_writer.h \
        ../libs/libmv/third_party/ceres/internal/ceres/block_jacobi_preconditioner.h \
        ../libs/libmv/third_party/ceres/internal/ceres/block_random_access_dense_matrix.h \
        ../libs/libmv/third_party/ceres/internal/ceres/block_random_access_diagonal_matrix.h \
        ../libs/libmv/third_party/ceres/internal/ceres/block_random_access_matrix.h \
        ../libs/libmv/third_party/ceres/internal/ceres/block_random_access_sparse_matrix.h \
        ../libs/libmv/third_party/ceres/internal/ceres/block_sparse_matrix.h \
        ../libs/libmv/third_party/ceres/internal/ceres/block_structure.h \
        ../libs/libmv/third_party/ceres/internal/ceres/callbacks.h \
        ../libs/libmv/third_party/ceres/internal/ceres/canonical_views_clustering.h \
        ../libs/libmv/third_party/ceres/internal/ceres/casts.h \
        ../libs/libmv/third_party/ceres/internal/ceres/cgnr_linear_operator.h \
        ../libs/libmv/third_party/ceres/internal/ceres/cgnr_solver.h \
        ../libs/libmv/third_party/ceres/internal/ceres/collections_port.h \
        ../libs/libmv/third_party/ceres/internal/ceres/compressed_col_sparse_matrix_utils.h \
        ../libs/libmv/third_party/ceres/internal/ceres/compressed_row_jacobian_writer.h \
        ../libs/libmv/third_party/ceres/internal/ceres/compressed_row_sparse_matrix.h \
        ../libs/libmv/third_party/ceres/internal/ceres/conjugate_gradients_solver.h \
        ../libs/libmv/third_party/ceres/internal/ceres/coordinate_descent_minimizer.h \
        ../libs/libmv/third_party/ceres/internal/ceres/corrector.h \
        ../libs/libmv/third_party/ceres/internal/ceres/covariance_impl.h \
        ../libs/libmv/third_party/ceres/internal/ceres/cxsparse.h \
        ../libs/libmv/third_party/ceres/internal/ceres/dense_jacobian_writer.h \
        ../libs/libmv/third_party/ceres/internal/ceres/dense_normal_cholesky_solver.h \
        ../libs/libmv/third_party/ceres/internal/ceres/dense_qr_solver.h \
        ../libs/libmv/third_party/ceres/internal/ceres/dense_sparse_matrix.h \
        ../libs/libmv/third_party/ceres/internal/ceres/detect_structure.h \
        ../libs/libmv/third_party/ceres/internal/ceres/dogleg_strategy.h \
        ../libs/libmv/third_party/ceres/internal/ceres/dynamic_compressed_row_finalizer.h \
        ../libs/libmv/third_party/ceres/internal/ceres/dynamic_compressed_row_jacobian_writer.h \
        ../libs/libmv/third_party/ceres/internal/ceres/dynamic_compressed_row_sparse_matrix.h \
        ../libs/libmv/third_party/ceres/internal/ceres/evaluator.h \
        ../libs/libmv/third_party/ceres/internal/ceres/execution_summary.h \
        ../libs/libmv/third_party/ceres/internal/ceres/file.h \
        ../libs/libmv/third_party/ceres/internal/ceres/gradient_checking_cost_function.h \
        ../libs/libmv/third_party/ceres/internal/ceres/gradient_problem_evaluator.h \
        ../libs/libmv/third_party/ceres/internal/ceres/graph_algorithms.h \
        ../libs/libmv/third_party/ceres/internal/ceres/graph.h \
        ../libs/libmv/third_party/ceres/internal/ceres/implicit_schur_complement.h \
        ../libs/libmv/third_party/ceres/internal/ceres/incomplete_lq_factorization.h \
        ../libs/libmv/third_party/ceres/internal/ceres/integral_types.h \
        ../libs/libmv/third_party/ceres/internal/ceres/iterative_schur_complement_solver.h \
        ../libs/libmv/third_party/ceres/internal/ceres/lapack.h \
        ../libs/libmv/third_party/ceres/internal/ceres/levenberg_marquardt_strategy.h \
        ../libs/libmv/third_party/ceres/internal/ceres/linear_least_squares_problems.h \
        ../libs/libmv/third_party/ceres/internal/ceres/linear_operator.h \
        ../libs/libmv/third_party/ceres/internal/ceres/linear_solver.h \
        ../libs/libmv/third_party/ceres/internal/ceres/line_search_direction.h \
        ../libs/libmv/third_party/ceres/internal/ceres/line_search.h \
        ../libs/libmv/third_party/ceres/internal/ceres/line_search_minimizer.h \
        ../libs/libmv/third_party/ceres/internal/ceres/line_search_preprocessor.h \
        ../libs/libmv/third_party/ceres/internal/ceres/low_rank_inverse_hessian.h \
        ../libs/libmv/third_party/ceres/internal/ceres/map_util.h \
        ../libs/libmv/third_party/ceres/internal/ceres/minimizer.h \
        ../libs/libmv/third_party/ceres/internal/ceres/mutex.h \
        ../libs/libmv/third_party/ceres/internal/ceres/parameter_block.h \
        ../libs/libmv/third_party/ceres/internal/ceres/parameter_block_ordering.h \
        ../libs/libmv/third_party/ceres/internal/ceres/partitioned_matrix_view.h \
        ../libs/libmv/third_party/ceres/internal/ceres/partitioned_matrix_view_impl.h \
        ../libs/libmv/third_party/ceres/internal/ceres/polynomial.h \
        ../libs/libmv/third_party/ceres/internal/ceres/preconditioner.h \
        ../libs/libmv/third_party/ceres/internal/ceres/preprocessor.h \
        ../libs/libmv/third_party/ceres/internal/ceres/problem_impl.h \
        ../libs/libmv/third_party/ceres/internal/ceres/program_evaluator.h \
        ../libs/libmv/third_party/ceres/internal/ceres/program.h \
        ../libs/libmv/third_party/ceres/internal/ceres/random.h \
        ../libs/libmv/third_party/ceres/internal/ceres/reorder_program.h \
        ../libs/libmv/third_party/ceres/internal/ceres/residual_block.h \
        ../libs/libmv/third_party/ceres/internal/ceres/residual_block_utils.h \
        ../libs/libmv/third_party/ceres/internal/ceres/schur_complement_solver.h \
        ../libs/libmv/third_party/ceres/internal/ceres/schur_eliminator.h \
        ../libs/libmv/third_party/ceres/internal/ceres/schur_eliminator_impl.h \
        ../libs/libmv/third_party/ceres/internal/ceres/schur_jacobi_preconditioner.h \
        ../libs/libmv/third_party/ceres/internal/ceres/scratch_evaluate_preparer.h \
        ../libs/libmv/third_party/ceres/internal/ceres/single_linkage_clustering.h \
        ../libs/libmv/third_party/ceres/internal/ceres/small_blas.h \
        ../libs/libmv/third_party/ceres/internal/ceres/solver_utils.h \
        ../libs/libmv/third_party/ceres/internal/ceres/sparse_matrix.h \
        ../libs/libmv/third_party/ceres/internal/ceres/sparse_normal_cholesky_solver.h \
        ../libs/libmv/third_party/ceres/internal/ceres/split.h \
        ../libs/libmv/third_party/ceres/internal/ceres/stl_util.h \
        ../libs/libmv/third_party/ceres/internal/ceres/stringprintf.h \
        ../libs/libmv/third_party/ceres/internal/ceres/suitesparse.h \
        ../libs/libmv/third_party/ceres/internal/ceres/triplet_sparse_matrix.h \
        ../libs/libmv/third_party/ceres/internal/ceres/trust_region_minimizer.h \
        ../libs/libmv/third_party/ceres/internal/ceres/trust_region_preprocessor.h \
        ../libs/libmv/third_party/ceres/internal/ceres/trust_region_strategy.h \
        ../libs/libmv/third_party/ceres/internal/ceres/visibility_based_preconditioner.h \
        ../libs/libmv/third_party/ceres/internal/ceres/visibility.h \
        ../libs/libmv/third_party/ceres/internal/ceres/wall_time.h
