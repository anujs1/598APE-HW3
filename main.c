#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>

#ifdef _OPENMP
#include <omp.h>
#endif

float tdiff(struct timeval *start, struct timeval *end)
{
  return (end->tv_sec - start->tv_sec) + 1e-6 * (end->tv_usec - start->tv_usec);
}

typedef struct {
  uint64_t state;
} rng_state_t;

static inline uint64_t rng_next(rng_state_t *rng)
{
  rng->state ^= (rng->state << 21);
  rng->state ^= (rng->state >> 35);
  rng->state ^= (rng->state << 4);
  return rng->state;
}

static inline double rng_double(rng_state_t *rng)
{
  uint64_t x = rng_next(rng);
  uint64_t r = x >> 11; /* top 53 bits */
  return r * (1.0 / 9007199254740992.0);
}

int L;          // Lattice size (L x L)
double T;       // Temperature
double J = 1.0; // Coupling constant
int8_t *spins;
#define IDX(i, j) ((i) * L + (j))
double total_energy = 0.0;
double accept_prob[3] = {1.0, 0.0, 0.0};

void initializeLattice(rng_state_t *rng)
{
  spins = (int8_t *)malloc((size_t)L * (size_t)L * sizeof(int8_t));
  for (int i = 0; i < L; i++)
  {
    for (int j = 0; j < L; j++)
    {
      spins[IDX(i, j)] = (rng_double(rng) < 0.5) ? -1 : 1;
    }
  }
}

double calculateTotalEnergy()
{
  double energy = 0.0;

  for (int i = 0; i < L; i++)
  {
    for (int j = 0; j < L; j++)
    {
      int spin = spins[IDX(i, j)];

      int up = spins[IDX((i - 1 + L) % L, j)];
      int down = spins[IDX((i + 1) % L, j)];
      int left = spins[IDX(i, (j - 1 + L) % L)];
      int right = spins[IDX(i, (j + 1) % L)];

      energy += -J * spin * (up + down + left + right);
    }
  }
  return 0.5 * energy;
}

double calculateMagnetization()
{
  double mag = 0.0;
  for (int i = 0; i < L; i++)
  {
    for (int j = 0; j < L; j++)
    {
      mag += spins[IDX(i, j)];
    }
  }
  return mag / (L * L);
}

void checkerboardSweep(int color, rng_state_t *thread_rngs, int num_threads)
{
  #pragma omp parallel num_threads(num_threads)
  {
    int tid = 0;
    #ifdef _OPENMP
        tid = omp_get_thread_num();
    #endif
    rng_state_t *rng = &thread_rngs[tid];
    
    #pragma omp for schedule(static) reduction(+:total_energy)
    for (int i = 0; i < L; i++)
    {
      for (int j = 0; j < L; j++)
      {
        if ((i + j) % 2 != color) continue;
        
        int spin = spins[IDX(i, j)];
        int up = spins[IDX((i - 1 + L) % L, j)];
        int down = spins[IDX((i + 1) % L, j)];
        int left = spins[IDX(i, (j - 1 + L) % L)];
        int right = spins[IDX(i, (j + 1) % L)];
        int neighbor_sum = up + down + left + right;
        double dE = 2.0 * J * (double)spin * (double)neighbor_sum;
        
        if (dE <= 0.0)
        {
          spins[IDX(i, j)] = -spin;
          total_energy += dE;
        }
        else
        {
          int sabs_idx = abs(neighbor_sum) / 2;
          double prob = accept_prob[sabs_idx];
          if (rng_double(rng) < prob)
          {
            spins[IDX(i, j)] = -spin;
            total_energy += dE;
          }
        }
      }
    }
  }
}

void saveLatticeImage(const char *png_filename)
{
  char ppm_filename[256];
  snprintf(ppm_filename, sizeof(ppm_filename), "temp_%s.ppm", png_filename);

  FILE *f = fopen(ppm_filename, "wb");
  if (!f)
  {
    printf("Error: Could not create temporary file %s\n", ppm_filename);
    return;
  }

  fprintf(f, "P6\n");
  fprintf(f, "%d %d\n", L, L);
  fprintf(f, "255\n");

  for (int i = 0; i < L; i++)
  {
    for (int j = 0; j < L; j++)
    {
      unsigned char r, g, b;
      if (spins[IDX(i, j)] == 1)
      {
        r = 255;
        g = 255;
        b = 255;
      }
      else
      {
        r = 0;
        g = 50;
        b = 200;
      }
      fwrite(&r, 1, 1, f);
      fwrite(&g, 1, 1, f);
      fwrite(&b, 1, 1, f);
    }
  }

  fclose(f);

  char cmd[512];
  snprintf(cmd, sizeof(cmd), "convert %s %s 2>/dev/null", ppm_filename,
           png_filename);
  int result = system(cmd);

  if (result == 0)
  {
    printf("Saved visualization to %s\n", png_filename);
    remove(ppm_filename);
  }
  else
  {
    rename(ppm_filename, png_filename);
    printf("Saved visualization to %s (install ImageMagick for PNG)\n",
           png_filename);
  }
}

void sanityCheck(double energy, double mag_per_spin, const char *stage)
{
  double energy_per_spin = energy / (L * L);
  double Tc = 2.0 * J / log(1.0 + sqrt(2.0));

  printf("Sanity check [%s]:\n", stage);

  // 1. Energy per spin
  if (energy_per_spin < -2.0 * J - 0.01 || energy_per_spin > 2.0 * J + 0.01)
  {
    printf("  [ERROR] Energy per spin (%.4f) outside expected bounds "
           "[%.2f, %.2f]\n",
           energy_per_spin, -2.0 * J, 2.0 * J);
  }
  else
  {
    printf("  [OK] Energy per spin = %.4f (within bounds [%.2f, %.2f])\n",
           energy_per_spin, -2.0 * J, 2.0 * J);
  }

  // 2. Magnetization per spin
  if (fabs(mag_per_spin) > 1.01)
  {
    printf("  [ERROR] Magnetization per spin (%.4f) outside physical bounds "
           "[-1, 1]\n",
           mag_per_spin);
  }
  else
  {
    printf("  [OK] Magnetization per spin = %.4f (within bounds [-1, 1])\n",
           mag_per_spin);
  }

  printf("\n");
}

void freeLattice()
{
  free(spins);
}

int main(int argc, const char **argv)
{
  if (argc < 4)
  {
    printf("Usage: %s <lattice_size> <temperature> <steps>\n", argv[0]);
    printf("Example: %s 100 2.269 10000000\n", argv[0]);
    printf("\n2D Ising Model\n");
    printf("Critical temperature: Tc = 2J/ln(1+√2) ≈ 2.26918531421\n");
    return 1;
  }

  L = atoi(argv[1]);
  T = atof(argv[2]);
  int steps = atoi(argv[3]);
  
  int total_spins = L * L;
  int sweeps = (steps + total_spins - 1) / total_spins;
  if (sweeps < 1) sweeps = 1;

  int num_threads = 1;
  #ifdef _OPENMP
    #pragma omp parallel
    {
      #pragma omp single
      num_threads = omp_get_num_threads();
    }
  #endif

  printf("2D Ising Model\n");
  printf("=================================================\n");
  printf("Lattice size: %d x %d (%d spins)\n", L, L, L * L);
  printf("Temperature: T = %.4f (Tc ≈ 2.269)\n", T);
  printf("Coupling constant: J = %.2f\n", J);
  printf("Single-spin steps requested: %d\n", steps);
  printf("Equivalent Monte Carlo sweeps: %d\n", sweeps);
  printf("OpenMP threads: %d\n", num_threads);
  printf("=================================================\n\n");

  rng_state_t *thread_rngs = (rng_state_t *)malloc(num_threads * sizeof(rng_state_t));
  for (int i = 0; i < num_threads; i++)
  {
    thread_rngs[i].state = 100 + i * 12345;
  }

  initializeLattice(&thread_rngs[0]);
  double initial_energy = calculateTotalEnergy();
  total_energy = initial_energy;
  accept_prob[0] = 1.0;
  accept_prob[1] = exp(-4.0 * J / T);
  accept_prob[2] = exp(-8.0 * J / T);
  double initial_mag = calculateMagnetization();
  printf("Initial energy: %.4f\n", initial_energy);
  printf("Initial magnetization: %.4f\n\n", initial_mag);

  sanityCheck(initial_energy, initial_mag, "Initial state");

  saveLatticeImage("initial_state.png");

  struct timeval start, end;
  gettimeofday(&start, NULL);

  for (int step = 0; step < sweeps; step++)
  {
    checkerboardSweep(0, thread_rngs, num_threads);
    checkerboardSweep(1, thread_rngs, num_threads);
  }

  gettimeofday(&end, NULL);

  double final_energy = calculateTotalEnergy();
  double final_mag = calculateMagnetization();

  printf("\nFinal energy: %.4f\n", final_energy);
  printf("Final magnetization: %.4f\n\n", final_mag);

  sanityCheck(final_energy, final_mag, "Final state");

  printf("Total time: %0.6f seconds\n\n", tdiff(&start, &end));

  saveLatticeImage("final_state.png");

  free(thread_rngs);
  freeLattice();
  return 0;
}
