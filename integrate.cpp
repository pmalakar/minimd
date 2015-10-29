/* ----------------------------------------------------------------------
   miniMD is a simple, parallel molecular dynamics (MD) code.   miniMD is
   an MD microapplication in the Mantevo project at Sandia National
   Laboratories ( http://www.mantevo.org ). The primary
   authors of miniMD are Steve Plimpton (sjplimp@sandia.gov) , Paul Crozier
   (pscrozi@sandia.gov) and Christian Trott (crtrott@sandia.gov).

   Copyright (2008) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This library is free software; you
   can redistribute it and/or modify it under the terms of the GNU Lesser
   General Public License as published by the Free Software Foundation;
   either version 3 of the License, or (at your option) any later
   version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this software; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.  See also: http://www.gnu.org/licenses/lgpl.txt .

   For questions, contact Paul S. Crozier (pscrozi@sandia.gov) or
   Christian Trott (crtrott@sandia.gov).

   Please read the accompanying README and LICENSE files.
---------------------------------------------------------------------- */
//#define PRINTDEBUG(a) a
#define PRINTDEBUG(a)
#include "stdio.h"
#include "integrate.h"
//#include "output.h"
#include "openmp.h"
#include "math.h"
#include <mpi.h>

Integrate::Integrate() {sort_every=20;}
Integrate::~Integrate() {}

void Integrate::setup()
{
  dtforce = 0.5 * dt;
}

void Integrate::initialIntegrate()
{
  OMPFORSCHEDULE
  for(MMD_int i = 0; i < nlocal; i++) {
    v[i * PAD + 0] += dtforce * f[i * PAD + 0];
    v[i * PAD + 1] += dtforce * f[i * PAD + 1];
    v[i * PAD + 2] += dtforce * f[i * PAD + 2];
    x[i * PAD + 0] += dt * v[i * PAD + 0];
    x[i * PAD + 1] += dt * v[i * PAD + 1];
    x[i * PAD + 2] += dt * v[i * PAD + 2];
  }
}

void Integrate::finalIntegrate()
{
  OMPFORSCHEDULE
  for(MMD_int i = 0; i < nlocal; i++) {
    v[i * PAD + 0] += dtforce * f[i * PAD + 0];
    v[i * PAD + 1] += dtforce * f[i * PAD + 1];
    v[i * PAD + 2] += dtforce * f[i * PAD + 2];
  }

}

void Integrate::run(Atom &atom, Force* force, Neighbor &neighbor,
                    Comm &comm, Thermo &thermo, Timer &timer)
{
  int i, n;

	char dumpfile[] = "dump.txt";
	FILE *dumpfp = fopen (dumpfile, "w+");

  comm.timer = &timer;
  timer.array[TIME_TEST] = 0.0;

  int check_safeexchange = comm.check_safeexchange;

  mass = atom.mass;
  dtforce = dtforce / mass;
  //Use OpenMP threads only within the following loop containing the main loop.
  //Do not use OpenMP for setup and postprocessing.
  #pragma omp parallel private(i,n)
  {
    int next_sort = sort_every>0?sort_every:ntimes+1;

    for(n = 0; n < ntimes; n++) {

      #pragma omp barrier

      x = atom.x;
      v = atom.v;
      f = atom.f;
      xold = atom.xold;
      nlocal = atom.nlocal;

      initialIntegrate();

      #pragma omp master
      timer.stamp();

      if((n + 1) % neighbor.every) {

        comm.communicate(atom);
        #pragma omp master
        timer.stamp(TIME_COMM);

      } else {
        //these routines are not yet ported to OpenMP
        {
          if(check_safeexchange) {
            #pragma omp master
            {
              double d_max = 0;

              for(i = 0; i < atom.nlocal; i++) {
                double dx = (x[i * PAD + 0] - xold[i * PAD + 0]);

                if(dx > atom.box.xprd) dx -= atom.box.xprd;

                if(dx < -atom.box.xprd) dx += atom.box.xprd;

                double dy = (x[i * PAD + 1] - xold[i * PAD + 1]);

                if(dy > atom.box.yprd) dy -= atom.box.yprd;

                if(dy < -atom.box.yprd) dy += atom.box.yprd;

                double dz = (x[i * PAD + 2] - xold[i * PAD + 2]);

                if(dz > atom.box.zprd) dz -= atom.box.zprd;

                if(dz < -atom.box.zprd) dz += atom.box.zprd;

                double d = dx * dx + dy * dy + dz * dz;

                if(d > d_max) d_max = d;
              }

              d_max = sqrt(d_max);

              if((d_max > atom.box.xhi - atom.box.xlo) || (d_max > atom.box.yhi - atom.box.ylo) || (d_max > atom.box.zhi - atom.box.zlo))
                printf("Warning: Atoms move further than your subdomain size, which will eventually cause lost atoms.\n"
                "Increase reneighboring frequency or choose a different processor grid\n"
                "Maximum move distance: %lf; Subdomain dimensions: %lf %lf %lf\n",
                d_max, atom.box.xhi - atom.box.xlo, atom.box.yhi - atom.box.ylo, atom.box.zhi - atom.box.zlo);

            }

          }


          #pragma omp master
          timer.stamp_extra_start();
          comm.exchange(atom);
          if(n+1>=next_sort) {
            atom.sort(neighbor);
            next_sort +=  sort_every;
          }
          comm.borders(atom);
          #pragma omp master
          {
            timer.stamp_extra_stop(TIME_TEST);
            timer.stamp(TIME_COMM);
          }

          if(check_safeexchange)
            for(int i = 0; i < PAD * atom.nlocal; i++) xold[i] = x[i];
        }

        #pragma omp barrier

        neighbor.build(atom);

        // #pragma omp barrier

        #pragma omp master
        timer.stamp(TIME_NEIGH);
      }

      force->evflag = (n + 1) % thermo.nstat == 0;
      force->compute(atom, neighbor, comm, comm.me);

      #pragma omp master
      timer.stamp(TIME_FORCE);

      if(neighbor.halfneigh && neighbor.ghost_newton) {
        comm.reverse_communicate(atom);

        #pragma omp master
        timer.stamp(TIME_COMM);
      }

      v = atom.v;
      f = atom.f;
      nlocal = atom.nlocal;

      #pragma omp barrier

      finalIntegrate();

/*
	  if (n == 1 || n == 100) printf("%d: CHECK BOX: %d %lf %lf %lf %lf %lf %lf\n", comm.me, n, atom.box.xlo, atom.box.xhi, atom.box.ylo, atom.box.yhi, atom.box.zlo, atom.box.zhi);
 * e.g. output
 * 		100: CHECK BOX: 1 2.099495 4.198990 8.397981 10.497476 8.397981 10.497476
		100: CHECK BOX: 100 2.099495 4.198990 8.397981 10.497476 8.397981 10.497476
 *
 */ 

	  int numAtoms;
	  double time;
	  MPI_Scan(&atom.nlocal,&numAtoms,1,MPI_INT,MPI_SUM,MPI_COMM_WORLD);
#ifdef DEBUG
      printf("%d: num atoms %d at %dth step\n", comm.me, numAtoms, n); 
	  double t = MPI_Wtime();
	  for(int i = 0; i < atom.nlocal ; i++) {
    	fprintf(dumpfp, "%d: %d: atom %d of %d positions %lf %lf %lf\n", comm.me, n, i, atom.nlocal, atom.x[i * PAD + 0], atom.x[i * PAD + 1], atom.x[i * PAD + 2]);
    	printf("%d: %d: atom %d of %d positions %d %lf | %d %lf | %d %lf\n", comm.me, n, i, atom.nlocal, i * PAD + 0, atom.x[i * PAD + 0], i * PAD + 1, atom.x[i * PAD + 1], i * PAD + 2, atom.x[i * PAD + 2]);
    	printf("%d: %d: atom %d of %d velocities %d %lf | %d %lf | %d %lf\n", comm.me, n, i, atom.nlocal, i * PAD + 0, atom.v[i * PAD + 0], i * PAD + 1, atom.v[i * PAD + 1], i * PAD + 2, atom.v[i * PAD + 2]);
	  }
	  t = MPI_Wtime() - t;
	  MPI_Reduce (&t, &time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

	  t = MPI_Wtime();
	  /*
  		if (comm.me == 0)
      		MPI_File_get_size(mpifh,&mpifo);
    	MPI_Bcast(&mpifo, 1, MPI_INT, 0, world);
 	  */
	  t = MPI_Wtime() - t;
	  MPI_Reduce (&t, &time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

#endif	
	  //if (n == 1)
		//	MPI_File_write_at_all(fh, offset, atom.x, 3*atom.nlocal, MPI_DOUBLE, MPI_STATUS_IGNORE);

      if(thermo.nstat) thermo.compute(n + 1, atom, neighbor, force, timer, comm);

    }
  } //end OpenMP parallel

	fclose(dumpfp);
}
