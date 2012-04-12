#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// for debugging
#define CONVERT(x) ( (*(uint64_t*)(&(x))) & 0xFFFFFFFF00000000 == 0x7FF4DEAD ?  (double)(*(float*)(&(x))) : (x) )
//#define CONVERT(x) (x)


{{fn foo MPI_Allreduce}}

  // we're only interested in doubles
  if (datatype != MPI_DOUBLE && datatype != MPI_DOUBLE_PRECISION) {
    printf("MPI_Allreduce non-MPI_DOUBLE\n");
    {{callfn}}
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

{{endfn}}


{{fn foo MPI_Reduce}}

  // we're only interested in doubles
  if (datatype != MPI_DOUBLE && datatype != MPI_DOUBLE_PRECISION) {
    printf("MPI_Reduce non-MPI_DOUBLE\n");
    {{callfn}}
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

{{endfn}}
