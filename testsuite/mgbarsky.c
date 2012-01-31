/*  
 * This program is to solve a nonlinear 2-D pde using a one-step linearization 
 *  
 *  It's multigrid, but not full MG. That is, I start at the finest level.
 *  It's written in C, but a fairly transparent version.
 *
 *  the equation I am solving is :
 *     (d/dt)u = Nu+f(x,y,t), where N is a slightly non-linear operator.
 *     N = (\nabla)^2 + function (u)  where \nabla is latex for 
 *          (d^2/dx^2 + d^2/dy^2)
 *     (d/dt)u is obviously a single (time) derivative,
 *     and function(u) is the nonlinear function of u, (no derivatives of u).
 *     the function f(x,y,t) has mostly been chosen to test the code for a known
 *     outcome, but can be any 'nice' function.
 *   The system is 2-dimensional, and has periodic boundary conditions.
 *
 *   N can be any function of u that is continuous on the interval. I have only
 *   rigorously tested for powers of u, exponentials and some trigonometric 
 *   functions. I haven't tried anything else.
 *   
 *   The non-linearity is handled by a one-step linearization at the finest
 *    grid level, and the resulting linear eqn is solved using MG techniques.
 *    See, for example, the book 'Multigrid' by Trottenberg, Oosterlee, Schuller
 *    section 5.3.2 and 5.3.3. The exact eqn solved is explained in the
 *    function mgrid.
 *   There is also a newton relaxation step performed at the finest level,
 *    see for example 'Numerical Methods for Differential Equations' by
 *    Ortega and Poole section 4.4 for a nice introduction.
 *
 *   I've mostly written this as a tutorial code. To that end, I've tried to
 *    explain what's going on throughout the code. The reader is expected to 
 *    have a little familiarity with linear MG and finite difference methods.
 *    And, it's not terribly optimized, as I've tried to keep things transparent
 *    I'll try to provide some hints for a faster code, but profile it & go from
 *    there.
 *
 *   The periodic boundary conditions have a tremendous effect throughout the
 *    code. In particular if you wanted to change to Dirichelet or Neumann
 *    boundary conditions
 *    you would need to change all the loops to (i=1;i<n;i++), assuming the 
 *    boundary was at 0 and n. You'd also have to call the boundary conditions
 *    at every time step, if they are functions of time. I've also assumed 
 *    solution on a square, but that is easily generalizable.
 *
 *    I use the Crank-Nicolson algorithm to integrate in time. It solves the
 *     following equation:
 *      [u(t)-u(t-1)]/tau = (1/2)*[Nu(t) + f(x,y,t) + Nu(t-1) + f(x,y,t-1)]
 *      or:
 *     Nu(t) -(2/tau)u(t)= -[Nu(t-1)+(2/tau) u(t-1)+f(x,y,t-1) + f(x,y,t)]  (**)
 *      where t is the timestep we're now on, and t-1 is the previous time.
 *      Thus (**) is the system we're really solving, and everything on 
 *      the right hand side of the equation is known.
 *      Now, the PDE you solve is  Nu -(2/tau) u = F, where F is the right-hand
 *      side of (**). You will need to hand-tune F, in particular f(x,y,t-1) and
 *      f(x,y,t) for every different equation you run.
 *
 *    
 *    If you don't want to solve a time-dependent PDE, but merely wish to solve
 *      a PDE, ie Nu = f, you'll need to change the routine 'funct' to contain
 *      only f, and remove the while (t<timemax) loop in main.
 *
 *    Debugging. There may well be errors left in the code. Most of the problems
 *      I've had had to do with solving the eqns improperly based on the initial
 *      conditions, and the hand-tuning bit in funct, ie properly calculating
 *      f(x,y,t) based on the initial conditions and the nonlinearity.
 *
 *    This program has mostly been written by Sandra Barsky, with help 
 *      (especially with periodic BC's) from Ralph Giles. My understanding 
 *      of MG techniques is imperfect at best, and there may remain errors 
 *      in this code. If you'd like to point out errors, or know of a way to
 *      improve the accuracy of the solution or make any other comments 
 *      (even if only to let me know you've looked at the program!) please 
 *      send to:    barsky@thaumas.net
 */    

//
#define invtau (-2./tau)  //crank-nicholson alog. : need this for linear term
#define tau   0.1  // timestep
#define nrep1 1    // number of pre-smoothing steps
#define nrep2 3    // number of post-smoothing steps
#define nmin 16    // minimum number of grid points
#define nmax 64    // maximum number of grid points
#define timemax 2 // maximum number of time steps

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define pi 4.0*atan(1.0) 
#define xmax pi   //  square I'm solving is [-pi:pi] in both x & y directions
#define xmin -pi  //

#define filesize 64


/* --------------------------------------------------------------------*/
/*  function definitions                                               */
/* --------------------------------------------------------------------*/


void interpolate(double  *uc,double  *u,int nc,int nf);
void restrct(double *uc, double *u,int n);
void restrct2(double *uc, double *u,int n);
void funct(double *f, double *u,int n, int t);
void ic(double *u,int n,int t); /* intial condition, not boundary condition */
double error(double *u,double *uback,int n,int *imx,int *jmx);

void pooofle(double *u, int n, double  *f, double *nlin);

void mgrid(double *u, int n, double *f, int t);
void defectnl(double *d, double *u,int n,double *f);
void newton(double *u, int n, double *d, double *v);

void newtonnl(double *u, int n, double *f);
int index2d(int i,int j, int n);
double errordefect(double *d, int n);


/* --------------------------------------------------------------------*/
/* fn is the nonlinear part of the operator N.
 * df is the derivative of (fn) with respect to u
 */
inline double fn( double u) {
  return(u-u*u*u);
}
inline double df( double u) {
  return(1.-3.*u*u);
}
/* --------------------------------------------------------------------*/

int main()
{
  double *areal; /* areal is the variable I'm solving for. will be called u
		    in the functions that are dependent on this on. I just
		    needed to give it a different name here */
  double *freal; /* freal is the right hand side of (**), ie F */
  double h, calc, act, err;
  int n,nmem,t;

  printf("== mgbarsky ==\n");

  n=nmax;
  nmem=(n+1)*(n+1);
  t=1;

  areal=malloc(nmem*sizeof(double));
  if(areal==NULL)printf(" oopsies can't allocate areal %d \n",__LINE__);
  ic(areal,n,t);//call initial condition if it's the first time through
 
  freal=malloc(nmem*sizeof(double));
  if(freal==NULL)printf(" oopsies can't allocate freal %d \n",__LINE__);


  while(t < timemax){

  /*int i;*/
  /*printf(" before funct:");*/
  /*for (i=64*64/2; i<64*64/2+4; i++) { printf(" %f", areal[i]); }*/
  /*for (i=64*64/2; i<64*64/2+4; i++) { printf(" %f", freal[i]); }*/
  /*printf("\n");*/

    funct(freal,areal,n,t);

  /*printf(" after funct:");*/
  /*for (i=64*64/2; i<64*64/2+4; i++) { printf(" %f", areal[i]); }*/
  /*for (i=64*64/2; i<64*64/2+4; i++) { printf(" %f", freal[i]); }*/
  /*printf("\n");*/
	
    mgrid(areal,nmax,freal,t);

    h=(xmax-xmin)/(double) n;
    calc = areal[index2d(n/4,n/4,n)];
    act = sin(t*tau);

    if (t%10==1) {
     printf("%3d: %8.4lf %8.4lf %8.4lf\n",t,calc,act, err);
    }
    fflush(stdout);

    t++;
  }/* while */

    if (t > 2) {
     printf("%3d: %8.4lf %8.4lf %8.4lf\n",t,calc,act, err);
    }
    err = fabs((calc-act)/act);
    if (!finite(err) || err > 0.001) {
        printf("== mgbarsky: FAIL!!!\n");
        return EXIT_FAILURE;
    }

    err = fabs((calc-0.0998)/0.0998);
    if (!finite(err) || err > 0.01) {
        printf("== mgbarsky: FAIL!!!\n");
        return EXIT_FAILURE;
    }
     

  free(areal);
  free(freal);

  printf("== mgbarsky: pass\n");
  return EXIT_SUCCESS;
}/* main */

void mgrid(double *u, int n,double *f,int t) 
{
  double *v,*d,*vc,*dc, *uf, *uc,*fc;
  double *uback,*uback2;
  double diffmx,diff,difv,x,y,tol,h;
  double dif,mxd,sumu,sumu2,sum3,sol;
  int i,j,k,imx,jmx,in,nc,irep;
  int ncmem,nsq,nmem;
  char filename[filesize];
  FILE *f1, *f2, *out, *f3, *f8;

  tol=10.e-12; // tol= tolerance. How accurately you want to solve the eqn


  diffmx=tol*5.;
  irep=0;

  nc=n/2;
  nmem=(n+1)*(n+1);  // bigger than [0:n]*[0:n], just to ensure enough space
  ncmem=(nc+1)*(nc+1);// same for the grid that is half-size
  nsq=n*n;  


/* --- requesting space for variables ------------------------------*/

  v=malloc(nmem*sizeof(double));
  if(v==NULL) {printf(" oopsies can't allocate v %d\n",__LINE__);}
  d=malloc(nmem*sizeof(double));
  if(d==NULL) printf(" oopsies can't allocate d %d\n",__LINE__);
  uback=malloc(nmem*sizeof(double));
  if(uback==NULL) printf(" oopsies can't allocate uback %d\n",__LINE__);
  uback2=malloc(nmem*sizeof(double));
  if(uback2==NULL) printf(" oopsies can't allocate uback2  %d\n",__LINE__);
  vc=malloc(ncmem*sizeof(double));
  if(vc==NULL) {printf(" oopsies can't allocate vc  %d\n",__LINE__);}
  uc=malloc(ncmem*sizeof(double));
  if(uc==NULL) {printf(" oopsies can't allocate uc  %d\n",__LINE__);}
  dc=malloc(ncmem*sizeof(double));
  if(dc==NULL) printf(" oopsies can't allocate dc  %d\n",__LINE__);


  for(i=0;i<nmem;i++){  // initialize to 0
    v[i]=0.;
    uback[i]=0.;
    d[i]=0.; }  // end of v,d,uback initialization
  for(i=0;i<ncmem;i++){
    vc[i]=0.;
    dc[i]=0.; }  // end of vc,dc



  h=(xmax-xmin)/(double) n;
  for(i=0;i<n;i++){
    x=xmin+h*i;
    for(j=0;j<n;j++){
      y=xmin+h*j;
      uback2[index2d(i,j,n)]=sin(x)*sin(y)*sin(t*tau);
    }//y     // the actual solution, to compare
  }//x

  diffmx=5*tol; /* tol determines solution tolerance. diffmx will measure 
                   that difference */

  irep=0;  // irep will be the number of MG steps required to solve


  /* +++++++++++++++ start of while loop ++++++++++++++++++*/

  while(diffmx>tol){

    diffmx=0.;   // need to zero this now, will measure later
    irep=irep+1; // counts number of MG steps

		 

    for( in=1;in<=nrep1;in++){  // presmoothing
      newtonnl(u,n,f);
    }//nrep1

    for( i=0;i<nmem;i++){
      uback[i]=u[i]; 
    }//uback=u

     /* defectnl calculates the residual. ie, given Nu=f
      *  d=f-Nu, or a measure of how far away you are from the solution */
        
    defectnl(d,u,n,f);

     /* restrict u, have a separate routine for it, in case you want to 
      * use a different restriction for than for other variables */

     restrct2(uc,u,nc);		 


    /* v is the linearlized version of u
     * given, Nu=f
     * define an operator K such that Kv=d, 
     * then u := u+v  (ie new u = old u+ v, v found from above
     * this can be derived from standard Newton's method:
     *  u := u - (Nu -f)/K
     *  where K=(d/du)N
     *
     *  so, if Nu = (\nabla)^2 u + g(u)
     *  K = (\nabla)^2 + (d/du)g
     *
     *  thus, the eqn we now solve is
     *
     *  Kv = (\nabla)^2v + (d/du)g v =d  (#)
     *
     *  and the next iterate of u is u+v
     *
     *  I apply MG techniques to (#)
     */

    restrct(vc,v,nc); 
    restrct(dc,d,nc); // dc is defect on coarse grid

    pooofle(vc,nc,dc,uc); /* funky multigrid part!
			     this solves for vc    */

    interpolate(vc,v,nc,n);//interpolate vc up to the fine grid

    for(i=1;i<2;i++){
      newton(u,n,d,v);   //   this is 'postsmoothing' for v !!
    }

    for( i=0;i<nmem;i++){
      u[i]+=v[i]; 
      uback[i]=u[i];
    }//u+=v

    for( i=0;i<nrep2;i++){
      newtonnl(u,n,f);   	// postsmoothing on u
    }//nrep2


    /* how to tell when you're done?
     * the classic way is to calculate the defect, and when the norm
     * of the defect is less than some predetermined accuracy (tolerance)
     * you're done.
     *
     * I have found that I get better accuracy - ie the solution remains
     * correct farther into time when I use as my condition the difference
     * between the current u, and the previous value of u. If anyone has
     * any ideas for this, please let me know !
     */

    defectnl(d,u,n,f);
    diff=errordefect(d,n)/n/n;

    diffmx=error(u,uback,n,&imx,&jmx); 
    /* diffmx is the condition to test against. for termination of
     * calculation based on defect, do:
     * diffmx=errordefect(d,n)/n/n;
     * diff=error(u,uback,n,&imx,&jmx);
     */ 


    mxd=-10.;
    sum3=0.;
    for(i=0;i<n;i++){
      for(j=0;j<n;j++){
        dif=uback2[index2d(i,j,n)]-u[index2d(i,j,n)]; 
	sum3+=u[index2d(i,j,n)];
        if(dif < 0.e-10) dif= -dif;  
        if(dif > mxd) {mxd= dif;  // maximum difference between actual &
	imx=i;                    // calculated solution. for fun
	jmx=j;                   // imx & jmx calculate where this happens
	} // end if
      }//j
    }//i

     

    if(irep>2000) break; // some break condition
  } /* while */

      
  /*
   * this will make a file with the x,y coordinates, the calculated &
   * actual solutions & their difference.
   *
   * uncomment if you want this printed out 
   *
  snprintf(filename,filesize,"solution.%d",t);
  f1=fopen(filename,"w");
  for(i=0;i<n;i++){
    x=xmin+h*i;
    for(j=0;j<n;j++){
      y=xmin+h*j;
      sol=sin(x)*sin(y)*sin(t*tau);
      dif=sol-u[index2d(i,j,n)];
      fprintf(f1,"%lf %lf %lf %lf %lf\n",x,y,u[index2d(i,j,n)],sol,dif,uback[index2d(i,j,n)]);
    }
  }
  fclose(f1);

  */

  free(uback);
  free(uback2);
  free(v);
  free(d);
  free(dc);
  free(uc);
  free(vc);

	     
} /* mg */ 


void ic(double *u,int n,int t) // initial condition
{
  int i,j;
  double x,y,h,timebefore;

  h=(xmax-xmin)/(double) n;

  timebefore=(double)(t-1)*tau;
  for(i=0;i<n;i++){
    x=i*h+xmin; 
    for(j=0;j<n;j++){
      y=j*h+xmin; 
      u[index2d(i,j,n)]=sin(x)*sin(y)*sin(timebefore);
    }//i
  }//j
}/* ic */

double error(double *u,double *uback, int n,int *imx,int *jmx)
{   double diff,diffmx,ave;
 int i,j,ic,k;		/* calculates the difference between u now &
			   u before, as one way to calculate error */
 ave=0;
 diffmx=0;
 ic=0;
 for(i=0;i<n;i++){
   for(j=0;j<n;j++){
     diff=u[index2d(i,j,n)]-uback[index2d(i,j,n)];
     if(diff < 0) diff=-diff;
     ave+=diff;
     ic+=1;
     if(diff > diffmx){
       diffmx=diff; 	
       *imx=i;
       *jmx=j;
     } //if
   }//j
 }//i
 ave=ave/ic;
 return (diffmx);
}/*error*/


void pooofle(double *u,int n,double *f,double *nlin)
{
  double *v,*uback,*dc,diff,tol,*nlinc;
  double *d,*vc,*uc,*fc,sumu,sumu2;
  int i,j,imx,jmx,nc;
  int nmem,ncmem,nfmem,count,w,nsq;
  FILE *f1;

  nmem=(n+1)*(n+1);
  nsq=n*n;

  for(i=0;i<nmem;i++){
    u[i]=0.;
  }

  nc=n/2;

  if(n > nmin){

    for(i=0;i<nrep1;i++){
      newton(nlin,n,f,u); // note that it is u that changes
    }       

    d=malloc(nmem*sizeof(double));
    if(d==NULL)printf(" oopsies can't allocate d %d\n",__LINE__);
    for( i=0;i<nmem;i++){ 
      d[i] =0.;
    }

    defectnl(d,u,n,f);
    ncmem=(nc+1)*(nc+1);

    dc=malloc(ncmem*sizeof(double));
    if(dc==NULL)printf(" oopsies can't allocate dc %d\n",__LINE__);
    nlinc=malloc(ncmem*sizeof(double));
    if(nlinc==NULL)printf(" oopsies can't allocate nlinc %d\n",__LINE__);
    vc=malloc(ncmem*sizeof(double));
    if(vc==NULL)printf(" oopsies can't allocate vc %d\n",__LINE__);
    for( i=0;i<ncmem;i++){ 
      dc[i] =0.;
      nlinc[i] =0.;
      vc[i] =0.;
    }


    restrct(dc,d,nc); /* dc is defect on coarse grid, just calculated it */
    restrct2(nlinc,nlin,nc);
    /* restricts the original function u to a coarser grid, since in the
     * linearized version of things need u in the operator K 
     * Here the original function u is called 'nlin'. confusing change
     * of notation
     */

    pooofle(vc,nc,dc,nlinc);

    free(dc);
    free(d);
    free(nlinc);

    v=malloc(nmem*sizeof(double));
    if(v==NULL) printf(" oopsies can't allocate v  %d\n",__LINE__);
    for( i=0;i<nmem;i++){ 
      v[i] =0.;
    }

    interpolate(vc,v,nc,n); /*  interpolate vc up to the fine grid */
    free(vc);

    for(i=0;i<nmem;i++){
      u[i]+=v[i];
    }

    free(v);

    for(i=0;i<nrep2;i++){ 	// postsmoothing
      newton(nlin,n,f,u);
    }
       
  }else{ /* ie do this when at the coarsest grid*/

    //      tol=10.e-10;
    //diff=5*tol;

    uback=malloc(nmem*sizeof(double));
    if(uback==NULL)printf(" oopsies can't allocate uback %d\n",__LINE__);
    //         while(diff > tol){    
    /* this is usually here in MG codes
     * it's the 'exact solution on coarse grid' bit
     * I've just found it makes no difference to 
     * the answer in the current code & it takes time
     *
     */
    diff=0;		
    sumu=0.;
    sumu2=0.;
    for(i=0;i<nsq;i++){		// need to get the avg of u to be 0 
      uback[i]=u[i];		// this is because of the periodic bc's
      sumu +=u[i];		// this acts as a 2nd boundary condition
    }/* i */
    sumu=sumu/nsq;
    for(i=0;i<nsq;i++){
      u[i] -= sumu ;
      //   sumu2 +=u[i];  // sum2 was just a check on u
    }/* i */

    newton(nlin,n,f,u);
    diff=error(u,uback,n,&imx,&jmx);

    //         }/* while */
    free(uback);
  }/* end if/else */
}/* end of poofle */


int index2d(int i,int  j,int  stride)	// this is the wrap-around condition for
{   					// the periodic boundaries
  /*printf("index2d: i=%d j=%d stride=%d\n", i, j, stride);*/
  if(i<0) i+=stride;
  else if(i>=stride) i-=stride;
  if(j<0) j+=stride;
  else if(j>=stride) j-=stride;
  return(i+stride*j);
}

void interpolate(double *uc,double *uf,int nc,int nf)
{
  int i1,i,j,j1,n;
  int ic,jc;
  /*       first equate the matching points on grid  */

  n=nc;
  for( ic=0;ic<nc;ic++){  /* remember uc,nc is coarse grid, uf nf is fine */
    for( jc=0;jc<nc;jc++){       
      i1=ic*2;
      j1=jc*2;
      uf[index2d(i1,j1,nf)]=uc[index2d(ic,jc,nc)]; /* for uf this is even i, even j */
    }//j1
  }//i1

  /* now match up i pts between defined from previous pts - these are 'odd i', even j */
  n=nf;
  for( i=0;i<n;i+=2){ /* i should increment by 2 */
    for( j=1;j<n;j+=2){       
      uf[index2d(i,j,nf)]=(uf[index2d(i,j-1,nf)] + uf[index2d(i,j+1,nf)])*0.5;
    }//j
  }//i

  for( i=1;i<n;i+=2){
    for( j=0;j<n;j++){
      uf[index2d(i,j,nf)]=(uf[index2d(i-1,j,nf)] + uf[index2d(i+1,j,nf)])*0.5; /** note that i,j has been reversed **/
    }//i
  }//j

}/* interpolate */

void restrct(double *uc, double *uf, int n)
{
  int i, j, i2, j2,nf;
  nf=2*n;
      
  /*first equate the matching points on grid */
  /*uc=coarse, uf=fine */
  for( i=0;i<n;i++){ // n is the # of pts on the coarse grid 
    for( j=0;j<n;j++) {
      i2=2*i;
      j2=2*j;
      uc[index2d(i,j,n)]=uf[index2d(i2,j2,nf)] *4.
	+2.*uf[index2d(i2,j2+1,nf)] +2.*uf[index2d(i2+1,j2,nf)]
	+2.*uf[index2d(i2-1,j2,nf)]+2.*uf[index2d(i2,j2-1,nf)]
	+uf[index2d(i2-1,j2+1,nf)] +uf[index2d(i2+1,j2-1,nf)]
	+uf[index2d(i2-1,j2-1,nf)] +uf[index2d(i2+1,j2+1,nf)];
      uc[index2d(i,j,n)]=uc[index2d(i,j,n)]/16.;

      // alternate schemes:
      //           uc[index2d(i,j,n)]=uf[index2d(i2,j2,nf)] ;
      //           or
      //           uc[index2d(i,j,n)]=uf[index2d(i2,j2,nf)] *4.
      //     +uf[index2d(i2,j2+1,nf)] +uf[index2d(i2+1,j2,nf)]
      //     +uf[index2d(i2-1,j2,nf)]+uf[index2d(i2,j2-1,nf)]
      //     uc[index2d(i,j,n)]=uc[index2d(i,j,n)]/8;
		    

    }//j
  }//i

} /* end  restrct */

void restrct2(double *uc, double *uf, int n)
{
  int i, j, i2, j2,nf;
  nf=2*n;
      
  for( i=0;i<n;i++){ /* n is the # of pts on the coarse grid */
    for( j=0;j<n;j++) {
      i2=2*i;
      j2=2*j;
      uc[index2d(i,j,n)]=uf[index2d(i2,j2,nf)]*4.
	+uf[index2d(i2,j2+1,nf)] +uf[index2d(i2+1,j2,nf)]
	+uf[index2d(i2-1,j2,nf)]+uf[index2d(i2,j2-1,nf)];
      uc[index2d(i,j,n)]=uc[index2d(i,j,n)]/8.;

    }/* loop over j */
  }/* loop over i */

} /* end  restrct2 */

double errordefect( double *d, int n) // calculates one norm of the 
	                             // matrix d
{
  int i,j;
  double diff;

  diff=0.;
  for(i=0; i<n; i++){
    for(j=0; j<n; j++){
      diff += d[index2d(i,j,n)]*d[index2d(i,j,n)];
    }
  }
  return(diff);
}/* errordefect */

void newton( double *u, int n, double *f, double *v)
{
  /* linearized newtons method for nonlinear pde  
   * see p. 149 trottenberg for example */ 

  int i,j,nmem;
  double h,h2,d2;
  double nonlin,denom,*v2;

  h=(xmax-xmin)/(double) n; 
  //this is h : h= (xmax-xmin)/n, assuming xmax/min=ymax/min
  h2=h*h;     //  if you change this, you must also change d2 in defect !!!
  d2=1./h2;

  for(i=0;i<n;i++){
    for(j=0;j<n;j++){
      denom= -4.*d2 +invtau +df(u[index2d(i,j,n)]);
      v[index2d(i,j,n)]=
	(f[index2d(i,j,n)]-(v[index2d(i-1,j,n)]+v[index2d(i,j-1,n)]
       + v[index2d(i+1,j,n)]+v[index2d(i,j+1,n)] )*d2 )/denom;
    }
  }

}/*newton*/


void defectnl(double *d, double *u,int n,double *f) 
{
  int i,j;
  double h,h2,d2,temp,temp1,temp2,temp3,b1;

  h=(xmax-xmin)/(double) n; //this is h : h= (xmax-xmin)/n
  h2=h*h;
  d2=1./h2;
  for( i=0;i<n;i++){ 
    for( j=0;j<n;j++){  // d = f - Nu
      d[index2d(i,j,n)]=f[index2d(i,j,n)] -
	+ (  (u[index2d(i-1,j,n)]+u[index2d(i,j-1,n)]
	      +u[index2d(i+1,j,n)]+u[index2d(i,j+1,n)] -4*u[index2d(i,j,n)])*d2
	     +invtau*u[index2d(i,j,n)]
	     +fn(u[index2d(i,j,n)]) );

    }
  }
}/* defectnl */

void funct(double *f,double *u,int n, int t)
{
  int i,j,k;
  double x,y,z,h,t1 /*,tmp*/;
  double d2,sxsy,sint,sint1,uij,fij,fm1,fm,cost,cost1;
  /*printf("n=%d t=%d xmax=%lg xmin=%lg\n", n, t, xmax, xmin);*/
  t1=(t-1)*tau;
    /*printf("t1=%lg\n", t1);*/
  /*tmp=xmax-xmin;*/
    /*printf("tmp=%lg\n", tmp);*/
  d2=(double)n/(xmax-xmin);
  /*d2=(double)n;*/
    /*printf("d2=%lg\n", d2);*/
  /*d2=d2/tmp;*/
    /*printf("d2=%lg\n", d2);*/
  d2=d2*d2;
    /*printf("d2=%lg\n", d2);*/
  h=(xmax-xmin)/(double) n;
    /*printf("h=%lg\n", h);*/
  for(i=0;i<n;i++){
    x=i*h +xmin;
        /*printf("x=%lg\n", x);*/
    for(j=0;j<n;j++){ 
        /*printf(" j=%d ", j);*/
      y=j*h +xmin;
        /*printf("y=%lg\n", y);*/
        /*printf("sin(x)=%lg\n", sin(x));*/
      sxsy=sin(x)*sin(y); // definitions to make things easier
        /*printf("sxsy=%lg\n", sxsy);*/
      sint=sin(t*tau);
        /*printf("sint=%lg\n", sint);*/
      sint1=sin(t1);
      cost=cos(t*tau);
        /*printf("cost=%lg\n", cost);*/
      cost1=cos(t1);
      uij=u[index2d(i,j,n)];
        /*printf("uij=%lg\n", uij);*/
        /*printf("sxsy=%lg cost=%lg sint=%lg\n", sxsy, cost, sint);*/
      // define the functions f^m+1, f^m here to makes things easier
      // eqn is : Nu +f = d_t u  => f=d_t u-Nu
      // need f at this time (t), and previous time (t-1)
      // see comments at begining for more explanation
      fm  = sxsy*cost   + 2*sxsy*sint  - fn(sxsy*sint);
      /*fm  = sxsy*cost;*/
        /*printf("sxsy=%lg cost=%lg fm=%lg\n", sxsy, cost, fm);*/
      /*fm += 2*sxsy*sint;*/
        /*printf("fm=%lg\n", fm);*/
      /*fm -= fn(sxsy*sint);*/
        /*printf("fm=%lg\n", fm);*/
      fm1 = sxsy*cost1 + 2*sxsy*sint1 - fn(sxsy*sint1);
        /*printf("fm=%lg fm1=%lg\n", fm, fm1);*/
      fij=(u[index2d(i+1,j,n)]+u[index2d(i-1,j,n)]+
	   u[index2d(i,j+1,n)]+u[index2d(i,j-1,n)] -4.*uij)*d2;
      fij+=uij*(2./tau) + fn(uij) + fm1 +fm;
      f[index2d(i,j,n)]=-fij;
        /*printf("fij=%lg f[ijn]=%lg\n", fij, f[index2d(i,j,n)]);*/
  
    }//j
  }//i
} /* funct */ 

void newtonnl( double *u, int n, double *f)
{ //newtons method directly for pde
  int i,j,nmem; double h,h2,d2,*u2;
  double nonlin,denom;

  h=(xmax-xmin)/(double) n;
  h2=h*h;     //  if you change this, you must also change d2 in defect !!!
  d2=1./h2;
  for(i=0;i<n;i++){ 
    for(j=0;j<n;j++){ 
      denom= -4.*d2 +invtau+df(u[index2d(i,j,n)]);
      nonlin=u[index2d(i,j,n)]*invtau +fn(u[index2d(i,j,n)]) -f[index2d(i,j,n)];
      u[index2d(i,j,n)]=
	u[index2d(i,j,n)]
	-( (u[index2d(i-1,j,n)]+u[index2d(i,j-1,n)]+ u[index2d(i+1,j,n)]
	   +u[index2d(i,j+1,n)] -4*u[index2d(i,j,n)])*d2 +nonlin )/denom;
    }//j
  }//i

}/*newtonnl*/
