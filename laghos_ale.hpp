// Copyright (c) 2017, Lawrence Livermore National Security, LLC. Produced at
// the Lawrence Livermore National Laboratory. LLNL-CODE-734707. All Rights
// reserved. See files LICENSE and NOTICE for details.
//
// This file is part of CEED, a collection of benchmarks, miniapps, software
// libraries and APIs for efficient high-order finite element and spectral
// element discretizations for exascale applications. For more information and
// source code availability see http://github.com/ceed.
//
// The CEED research is supported by the Exascale Computing Project 17-SC-20-SC,
// a collaborative effort of two U.S. Department of Energy organizations (Office
// of Science and the National Nuclear Security Administration) responsible for
// the planning and preparation of a capable exascale ecosystem, including
// software, applications, hardware, advanced system engineering and early
// testbed platforms, in support of the nation's exascale computing imperative.

#ifndef MFEM_LAGHOS_ALE
#define MFEM_LAGHOS_ALE

#include "mfem.hpp"

namespace mfem
{

namespace hydrodynamics
{

class SolutionMover;

// Performs the full remap advection loop.
class RemapAdvector
{
private:
   ParMesh pmesh;
   int dim;
   L2_FECollection fec_L2;
   H1_FECollection fec_H1;
   ParFiniteElementSpace pfes_L2, pfes_H1;

   // Remap state variables.
   Array<int> offsets;
   BlockVector S;
   ParGridFunction d, v, rho, e;

   RK3SSPSolver ode_solver;
   Vector x0;

public:
   RemapAdvector(const ParMesh &m, int order_v, int order_e);

   void InitFromLagr(const Vector &nodes0,
                     const ParGridFunction &dist, const ParGridFunction &v,
                     const IntegrationRule &rho_ir, const Vector &rhoDetJw);

   virtual void ComputeAtNewPosition(const Vector &new_nodes);

   void TransferToLagr(ParGridFunction &dist, ParGridFunction &vel,
                       const IntegrationRule &ir_rho, Vector &rhoDetJw);
};

// Performs a single remap advection step.
class AdvectorOper : public TimeDependentOperator
{
protected:
   const Vector &x0;
   Vector &x_now;
   GridFunction &u;
   VectorGridFunctionCoefficient u_coeff;
   mutable ParBilinearForm M, K;
   mutable ParBilinearForm M_L2, K_L2;
   double dt = 0.0;

   void ComputeElementsMinMax(const ParFiniteElementSpace &pfes,
                              const Vector &u,
                              Vector &el_min, Vector &el_max) const;
   void ComputeSparsityBounds(const ParFiniteElementSpace &pfes,
                              const Vector &el_min, const Vector &el_max,
                              Vector &u_min, Vector &u_max) const;

public:
   /** Here @a pfes is the ParFESpace of the function that will be moved. Note
       that Mult() moves the nodes of the mesh corresponding to @a pfes. */
   AdvectorOper(int size, const Vector &x_start, GridFunction &velocity,
                ParFiniteElementSpace &pfes_H1, ParFiniteElementSpace &pfes_L2);

   virtual void Mult(const Vector &U, Vector &dU) const;

   void SetDt(double delta_t) { dt = delta_t; }
};

// Transfers of data between Lagrange and remap phases.
class SolutionMover
{
   // Integration points for the density.
   const IntegrationRule &ir_rho;

public:
   SolutionMover(const IntegrationRule &ir) : ir_rho(ir) { }

   // Density Lagrange -> Remap.
   void MoveDensityLR(const Vector &quad_rho, ParGridFunction &rho);
};

class LocalInverseHOSolver
{
protected:
   ParBilinearForm &M, &K;

public:
   LocalInverseHOSolver(ParBilinearForm &Mbf, ParBilinearForm &Kbf)
      : M(Mbf), K(Kbf) { }

   void CalcHOSolution(const Vector &u, Vector &du) const;
};

class DiscreteUpwindLOSolver
{
protected:
   ParFiniteElementSpace &pfes;
   const SparseMatrix &K;
   mutable SparseMatrix D;

   Array<int> K_smap;
   const Vector &M_lumped;

   void ComputeDiscreteUpwindMatrix() const;
   void ApplyDiscreteUpwindMatrix(ParGridFunction &u, Vector &du) const;

public:
   DiscreteUpwindLOSolver(ParFiniteElementSpace &space, const SparseMatrix &adv,
                          const Vector &Mlump);

   virtual void CalcLOSolution(const Vector &u, Vector &du) const;
   Array<int> &GetKmap() { return K_smap; }
};

class FluxBasedFCT
{
protected:
   ParFiniteElementSpace &pfes;
   double dt;

   const SparseMatrix &K, &M;
   const Array<int> &K_smap;

   // Temporary computation objects.
   mutable SparseMatrix flux_ij;
   mutable ParGridFunction gp, gm;

   void ComputeFluxMatrix(const ParGridFunction &u, const Vector &du_ho,
                          SparseMatrix &flux_mat) const;
   void AddFluxesAtDofs(const SparseMatrix &flux_mat,
                        Vector &flux_pos, Vector &flux_neg) const;
   void ComputeFluxCoefficients(const Vector &u, const Vector &du_lo,
      const Vector &m, const Vector &u_min, const Vector &u_max,
      Vector &coeff_pos, Vector &coeff_neg) const;
   void UpdateSolutionAndFlux(const Vector &du_lo, const Vector &m,
      ParGridFunction &coeff_pos, ParGridFunction &coeff_neg,
      SparseMatrix &flux_mat, Vector &du) const;

public:
   FluxBasedFCT(ParFiniteElementSpace &space,
                double delta_t,
                const SparseMatrix &adv_mat, const Array<int> &adv_smap,
                const SparseMatrix &mass_mat)
      : pfes(space), dt(delta_t),
        K(adv_mat), M(mass_mat), K_smap(adv_smap), flux_ij(adv_mat),
        gp(&pfes), gm(&pfes) { }

   virtual void CalcFCTSolution(const ParGridFunction &u, const Vector &m,
                                const Vector &du_ho, const Vector &du_lo,
                                const Vector &u_min, const Vector &u_max,
                                Vector &du) const;
};

} // namespace hydrodynamics

} // namespace mfem

#endif // MFEM_LAGHOS_ALE
