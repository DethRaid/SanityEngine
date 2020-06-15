#include "dual_contouring.hpp"

// ReSharper disable once CppInconsistentNaming
namespace _detail {
    float adapt(const Int32& v0, const Int32& v1) {
        RX_ASSERT((v0 > 0) != (v1 > 0), "Numbers do not have opposide sign");

        return static_cast<float>(0 - v0) / static_cast<float>(v1 - v0);
    }

    // from https://github.com/nickgildea/qef/blob/master/qef.cl

    // minimal SVD implementation for calculating feature points from hermite data
    // public domain

    typedef float mat3x3[3][3];
    typedef float mat3x3_tri[6];

#define SVD_NUM_SWEEPS 5

    // SVD
    ////////////////////////////////////////////////////////////////////////////////

#define PSUEDO_INVERSE_THRESHOLD (0.1f)

    void svd_mul_matrix_vec(Vec3f* result, mat3x3 a, Vec3f b) {
        (*result).x = dot(Vec3f{a[0][0], a[0][1], a[0][2]}, b);
        (*result).y = dot(Vec3f{a[1][0], a[1][1], a[1][2]}, b);
        (*result).z = dot(Vec3f{a[2][0], a[2][1], a[2][2]}, b);
    }

    void givens_coeffs_sym(float a_pp, float a_pq, float a_qq, float* c, float* s) {
        if(a_pq == 0.f) {
            *c = 1.f;
            *s = 0.f;
            return;
        }
        float tau = (a_qq - a_pp) / (2.f * a_pq);
        float stt = sqrt(1.f + tau * tau);
        float tan = 1.f / ((tau >= 0.f) ? (tau + stt) : (tau - stt));
        *c = 1.0 / sqrt(1.f + tan * tan);
        *s = tan * (*c);
    }

    void svd_rotate_xy(float* x, float* y, float c, float s) {
        float u = *x;
        float v = *y;
        *x = c * u - s * v;
        *y = s * u + c * v;
    }

    void svd_rotateq_xy(float* x, float* y, float* a, float c, float s) {
        float cc = c * c;
        float ss = s * s;
        float mx = 2.0 * c * s * (*a);
        float u = *x;
        float v = *y;
        *x = cc * u - mx + ss * v;
        *y = ss * u + mx + cc * v;
    }

    void svd_rotate(mat3x3 vtav, mat3x3 v, int a, int b) {
        if(vtav[a][b] == 0.0)
            return;

        float c, s;
        givens_coeffs_sym(vtav[a][a], vtav[a][b], vtav[b][b], &c, &s);

        float x, y, z;
        x = vtav[a][a];
        y = vtav[b][b];
        z = vtav[a][b];
        svd_rotateq_xy(&x, &y, &z, c, s);
        vtav[a][a] = x;
        vtav[b][b] = y;
        vtav[a][b] = z;

        x = vtav[0][3 - b];
        y = vtav[1 - a][2];
        svd_rotate_xy(&x, &y, c, s);
        vtav[0][3 - b] = x;
        vtav[1 - a][2] = y;

        vtav[a][b] = 0.0;

        x = v[0][a];
        y = v[0][b];
        svd_rotate_xy(&x, &y, c, s);
        v[0][a] = x;
        v[0][b] = y;

        x = v[1][a];
        y = v[1][b];
        svd_rotate_xy(&x, &y, c, s);
        v[1][a] = x;
        v[1][b] = y;

        x = v[2][a];
        y = v[2][b];
        svd_rotate_xy(&x, &y, c, s);
        v[2][a] = x;
        v[2][b] = y;
    }

    void svd_solve_sym(mat3x3_tri a, Vec3f* sigma, mat3x3 v) {
        // assuming that A is symmetric: can optimize all operations for
        // the upper right triagonal
        mat3x3 vtav;
        vtav[0][0] = a[0];
        vtav[0][1] = a[1];
        vtav[0][2] = a[2];
        vtav[1][0] = 0.f;
        vtav[1][1] = a[3];
        vtav[1][2] = a[4];
        vtav[2][0] = 0.f;
        vtav[2][1] = 0.f;
        vtav[2][2] = a[5];

        // assuming V is identity: you can also pass a matrix the rotations
        // should be applied to. (U is not computed)
        for(int i = 0; i < SVD_NUM_SWEEPS; ++i) {
            svd_rotate(vtav, v, 0, 1);
            svd_rotate(vtav, v, 0, 2);
            svd_rotate(vtav, v, 1, 2);
        }

        *sigma = Vec3f{vtav[0][0], vtav[1][1], vtav[2][2]};
    }

    float svd_invdet(float x, float tol) { return (fabs(x) < tol || fabs(1.0 / x) < tol) ? 0.0 : (1.0 / x); }

    void svd_pseudoinverse(mat3x3 o, Vec3f sigma, mat3x3 v) {
        float d0 = svd_invdet(sigma.x, PSUEDO_INVERSE_THRESHOLD);
        float d1 = svd_invdet(sigma.y, PSUEDO_INVERSE_THRESHOLD);
        float d2 = svd_invdet(sigma.z, PSUEDO_INVERSE_THRESHOLD);

        o[0][0] = v[0][0] * d0 * v[0][0] + v[0][1] * d1 * v[0][1] + v[0][2] * d2 * v[0][2];
        o[0][1] = v[0][0] * d0 * v[1][0] + v[0][1] * d1 * v[1][1] + v[0][2] * d2 * v[1][2];
        o[0][2] = v[0][0] * d0 * v[2][0] + v[0][1] * d1 * v[2][1] + v[0][2] * d2 * v[2][2];
        o[1][0] = v[1][0] * d0 * v[0][0] + v[1][1] * d1 * v[0][1] + v[1][2] * d2 * v[0][2];
        o[1][1] = v[1][0] * d0 * v[1][0] + v[1][1] * d1 * v[1][1] + v[1][2] * d2 * v[1][2];
        o[1][2] = v[1][0] * d0 * v[2][0] + v[1][1] * d1 * v[2][1] + v[1][2] * d2 * v[2][2];
        o[2][0] = v[2][0] * d0 * v[0][0] + v[2][1] * d1 * v[0][1] + v[2][2] * d2 * v[0][2];
        o[2][1] = v[2][0] * d0 * v[1][0] + v[2][1] * d1 * v[1][1] + v[2][2] * d2 * v[1][2];
        o[2][2] = v[2][0] * d0 * v[2][0] + v[2][1] * d1 * v[2][1] + v[2][2] * d2 * v[2][2];
    }

    void svd_solve_ATA_ATb(mat3x3_tri ATA, Vec3f ATb, Vec3f* x) {
        mat3x3 V;
        V[0][0] = 1.f;
        V[0][1] = 0.f;
        V[0][2] = 0.f;
        V[1][0] = 0.f;
        V[1][1] = 1.f;
        V[1][2] = 0.f;
        V[2][0] = 0.f;
        V[2][1] = 0.f;
        V[2][2] = 1.f;

        Vec3f sigma = {0.f, 0.f, 0.f};
        svd_solve_sym(ATA, &sigma, V);

        // A = UEV^T; U = A / (E*V^T)
        mat3x3 Vinv;
        svd_pseudoinverse(Vinv, sigma, V);
        svd_mul_matrix_vec(x, Vinv, ATb);
    }

    void svd_vmul_sym(Vec3f* result, mat3x3_tri A, Vec3f v) {
        Vec3f A_row_x = {A[0], A[1], A[2]};

        (*result).x = dot(A_row_x, v);
        (*result).y = A[1] * v.x + A[3] * v.y + A[4] * v.z;
        (*result).z = A[2] * v.x + A[4] * v.y + A[5] * v.z;
    }

    // QEF
    ////////////////////////////////////////////////////////////////////////////////

    void qef_add(Vec3f n, Vec3f p, mat3x3_tri ATA, Vec3f* ATb, Vec4f* pointaccum) {
        ATA[0] += n.x * n.x;
        ATA[1] += n.x * n.y;
        ATA[2] += n.x * n.z;
        ATA[3] += n.y * n.y;
        ATA[4] += n.y * n.z;
        ATA[5] += n.z * n.z;

        float b = dot(p, n);
        (*ATb).x += n.x * b;
        (*ATb).y += n.y * b;
        (*ATb).z += n.z * b;

        (*pointaccum).x += p.x;
        (*pointaccum).y += p.y;
        (*pointaccum).z += p.z;
        (*pointaccum).w += 1.f;
    }

    float qef_calc_error(mat3x3_tri A, Vec3f x, Vec3f b) {
        Vec3f tmp;

        svd_vmul_sym(&tmp, A, x);
        tmp = b - tmp;

        return dot(tmp, tmp);
    }

    float qef_solve(mat3x3_tri ATA, Vec3f ATb, Vec4f pointaccum, Vec3f* x) {
        Vec3f masspoint = {pointaccum.x / pointaccum.w, pointaccum.y / pointaccum.w, pointaccum.z / pointaccum.w};

        Vec3f A_mp = {0.f, 0.f, 0.f};
        svd_vmul_sym(&A_mp, ATA, masspoint);
        A_mp = ATb - A_mp;

        svd_solve_ATA_ATb(ATA, A_mp, x);

        float error = qef_calc_error(ATA, *x, ATb);
        (*x) = *x + masspoint;

        return error;
    }

    Vec3f qef_solve_from_points(const Vec3f* positions, const Vec3f* normals, const size_t count, float* error) {
        Vec4f pointaccum = {0.f, 0.f, 0.f, 0.f};
        Vec3f ATb = {0.f, 0.f, 0.f};
        mat3x3_tri ATA = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f};

        for(int i = 0; i < count; ++i) {
            qef_add(normals[i], positions[i], ATA, &ATb, &pointaccum);
        }

        Vec3f solved_position = {0.f, 0.f, 0.f};
        *error = qef_solve(ATA, ATb, pointaccum, &solved_position);
        return solved_position;
    }

    Vec3f solve_qef(Uint32 x, Uint32 y, Uint32 z, const Rx::Vector<Vec3f>& vertices, const Rx::Vector<Vec3f>& normals) {
        constexpr float ERROR_THRESHOLD = 0.5f;
        float error;
        const auto solution = qef_solve_from_points(vertices.data(), normals.data(), vertices.size(), &error);
        if(error > ERROR_THRESHOLD) {
            return {static_cast<Float32>(x), static_cast<Float32>(y), static_cast<Float32>(x)};

        } else {
            return solution;
        }
    }
} // namespace _detail

Quad Quad::swap(const bool swap) {
    if(swap) {
        return Quad{.v1 = v4, .v2 = v3, .v3 = v2, .v4 = v1};
    } else {
        return Quad{v1, v2, v3, v4};
    }
}
