
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _EXTERN_C_
#ifdef __cplusplus
#define _EXTERN_C_ extern "C"
#else /* __cplusplus */
#define _EXTERN_C_
#endif /* __cplusplus */
#endif /* _EXTERN_C_ */

#ifdef MPICH_HAS_C2F
_EXTERN_C_ void *MPIR_ToPointer(int);
#endif // MPICH_HAS_C2F

#ifdef PIC
/* For shared libraries, declare these weak and figure out which one was linked
   based on which init wrapper was called.  See mpi_init wrappers.  */
#pragma weak pmpi_init
#pragma weak PMPI_INIT
#pragma weak pmpi_init_
#pragma weak pmpi_init__
#endif /* PIC */

_EXTERN_C_ void pmpi_init(MPI_Fint *ierr);
_EXTERN_C_ void PMPI_INIT(MPI_Fint *ierr);
_EXTERN_C_ void pmpi_init_(MPI_Fint *ierr);
_EXTERN_C_ void pmpi_init__(MPI_Fint *ierr);

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// for debugging
#define CONVERT(x) ( (*(uint64_t*)(&(x))) & 0xFFFFFFFF00000000 == 0x7FF4DEAD ?  (double)(*(float*)(&(x))) : (x) )
//#define CONVERT(x) (x)


/* ================== C Wrappers for MPI_Allreduce ================== */
_EXTERN_C_ int PMPI_Allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
_EXTERN_C_ int MPI_Allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) { 
    int _wrap_py_return_val = 0;


  // we're only interested in doubles
  if (datatype != MPI_DOUBLE && datatype != MPI_DOUBLE_PRECISION) {
    printf("MPI_Allreduce non-MPI_DOUBLE\n");
    _wrap_py_return_val = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
  } else {
    printf("MPI_Allreduce MPI_DOUBLE\n");

    int rank;
    MPI_Comm_rank (comm, &rank);
    if (rank == 0) {

      // master node

      int i, j, size;
      MPI_Comm_size (comm, &size);
      void* tempbuf = malloc(count*sizeof(double));
      assert(tempbuf != NULL);
      double* temp = (double*)tempbuf;
      double* recv = (double*)recvbuf;
      double t;

      // initialize results with master's data
      memcpy(recvbuf, sendbuf, count*sizeof(double));

      // read from each node
      for (i=1; i<size; i++) {
        MPI_Recv(tempbuf, count, datatype, i, 0, comm, MPI_STATUS_IGNORE);
        // TODO: check for errors?

        // update each element in the buffer
        for (j=0; j<count; j++) {
          if (op == MPI_SUM) {
            recv[j] += CONVERT(temp[j]);
          } else if (op == MPI_PROD) {
            recv[j] *= CONVERT(temp[j]);
          } else if (op == MPI_MIN) {
            t = CONVERT(temp[j]);
            if (t < recv[j]) recv[j] = t;
          } else if (op == MPI_MAX) {
            t = CONVERT(temp[j]);
            if (t > recv[j]) recv[j] = t;
          } else {
            printf("ERROR: Unhandled MPI collective operation!\n");
          }
        }
      }

      // FOR ALLREDUCE ONLY
      for (i=1; i<size; i++) {
        MPI_Send(recvbuf, count, datatype, i, 0, comm);
      }

    } else {

      // regular node

      MPI_Send(sendbuf, count, datatype, 0, 0, comm);

      // FOR ALLREDUCE ONLY
      MPI_Recv(recvbuf, count, datatype, 0, 0, comm, MPI_STATUS_IGNORE);
      // TODO: check for errors?

    }
  }

    return _wrap_py_return_val;
}

/* =============== Fortran Wrappers for MPI_Allreduce =============== */
static void MPI_Allreduce_fortran_wrapper(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) { 
    int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
    _wrap_py_return_val = MPI_Allreduce((void*)sendbuf, (void*)recvbuf, *count, (MPI_Datatype)(*datatype), (MPI_Op)(*op), (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
    _wrap_py_return_val = MPI_Allreduce((void*)sendbuf, (void*)recvbuf, *count, MPI_Type_f2c(*datatype), MPI_Op_f2c(*op), MPI_Comm_f2c(*comm));
#endif /* MPICH test */
    *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_ALLREDUCE(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) { 
    MPI_Allreduce_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, comm, ierr);
}

_EXTERN_C_ void mpi_allreduce(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) { 
    MPI_Allreduce_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, comm, ierr);
}

_EXTERN_C_ void mpi_allreduce_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) { 
    MPI_Allreduce_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, comm, ierr);
}

_EXTERN_C_ void mpi_allreduce__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr) { 
    MPI_Allreduce_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, comm, ierr);
}

/* ================= End Wrappers for MPI_Allreduce ================= */





/* ================== C Wrappers for MPI_Reduce ================== */
_EXTERN_C_ int PMPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
_EXTERN_C_ int MPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) { 
    int _wrap_py_return_val = 0;


  // we're only interested in doubles
  if (datatype != MPI_DOUBLE && datatype != MPI_DOUBLE_PRECISION) {
    printf("MPI_Reduce non-MPI_DOUBLE\n");
    _wrap_py_return_val = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
  } else {
    printf("MPI_Reduce MPI_DOUBLE\n");

    int rank;
    MPI_Comm_rank (comm, &rank);
    if (rank == 0) {

      // master node

      int i, j, size;
      MPI_Comm_size (comm, &size);
      void* tempbuf = malloc(count*sizeof(double));
      assert(tempbuf != NULL);
      double* temp = (double*)tempbuf;
      double* recv = (double*)recvbuf;
      double t;

      // initialize results with master's data
      memcpy(recvbuf, sendbuf, count*sizeof(double));

      // read from each node
      for (i=1; i<size; i++) {
        MPI_Recv(tempbuf, count, datatype, i, 0, comm, MPI_STATUS_IGNORE);
        // TODO: check for errors?

        // update each element in the buffer
        for (j=0; j<count; j++) {
          if (op == MPI_SUM) {
            recv[j] += CONVERT(temp[j]);
          } else if (op == MPI_PROD) {
            recv[j] *= CONVERT(temp[j]);
          } else if (op == MPI_MIN) {
            t = CONVERT(temp[j]);
            if (t < recv[j]) recv[j] = t;
          } else if (op == MPI_MAX) {
            t = CONVERT(temp[j]);
            if (t > recv[j]) recv[j] = t;
          } else {
            printf("ERROR: Unhandled MPI collective operation!\n");
          }
        }
      }

    } else {

      // regular node

      MPI_Send(sendbuf, count, datatype, 0, 0, comm);

    }
  }

    return _wrap_py_return_val;
}

/* =============== Fortran Wrappers for MPI_Reduce =============== */
static void MPI_Reduce_fortran_wrapper(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) { 
    int _wrap_py_return_val = 0;
#if (!defined(MPICH_HAS_C2F) && defined(MPICH_NAME) && (MPICH_NAME == 1)) /* MPICH test */
    _wrap_py_return_val = MPI_Reduce((void*)sendbuf, (void*)recvbuf, *count, (MPI_Datatype)(*datatype), (MPI_Op)(*op), *root, (MPI_Comm)(*comm));
#else /* MPI-2 safe call */
    _wrap_py_return_val = MPI_Reduce((void*)sendbuf, (void*)recvbuf, *count, MPI_Type_f2c(*datatype), MPI_Op_f2c(*op), *root, MPI_Comm_f2c(*comm));
#endif /* MPICH test */
    *ierr = _wrap_py_return_val;
}

_EXTERN_C_ void MPI_REDUCE(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) { 
    MPI_Reduce_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, root, comm, ierr);
}

_EXTERN_C_ void mpi_reduce(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) { 
    MPI_Reduce_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, root, comm, ierr);
}

_EXTERN_C_ void mpi_reduce_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) { 
    MPI_Reduce_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, root, comm, ierr);
}

_EXTERN_C_ void mpi_reduce__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr) { 
    MPI_Reduce_fortran_wrapper(sendbuf, recvbuf, count, datatype, op, root, comm, ierr);
}

/* ================= End Wrappers for MPI_Reduce ================= */



