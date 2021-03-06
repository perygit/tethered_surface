#include <stdio.h>
#include <stdlib.h>
#include <math.h>

float radio_giraton(void);
float energia_curvatura(void);

void index_vcnos_prox();

int index_plqta_dir0(int i);
int index_plqta_dir1(int i);
int index_plqta_prox(int i, int dir);


#define L 16
#define N L*L 
#define M 2*(L-1)*(L-1) 
#define K 1.1 // Kappa 
#define NF 24000 // nº de archivos
#define B NF // nº de bloques (Jack-Knife)

#define _prodesc(w,u) (w.a*u.a+w.b*u.b+w.c*u.c)

#define _escala(u,x) \
                         u.a *= (x);\
                         u.b *= (x);\
                         u.c *= (x);

#define _norma2(u)  (u.a*u.a + u.b*u.b + u.c*u.c)

#define _prodvec(u,v,w) \
                         u.a = v.b*w.c - v.c*w.b; \
                         u.b = v.c*w.a - v.a*w.c; \
                         u.c = v.a*w.b - v.b*w.a; 
#define _resta(u,v,w) \
                         u.a = v.a - w.a; \
                         u.b = v.b - w.b; \
                         u.c = v.c - w.c; 
#define _suma(u,v,w) \
			u.a = v.a + w.a; \
                        u.b = v.b + w.b; \
                        u.c = v.c + w.c; 	
#define _inverso(u,v) \
		        u.a = -v.a; \
                        u.b = -v.b; \
                        u.c = -v.c; 

typedef struct{float a,b,c;} vector;

vector x[N];
int v[N][6],p[N][6];// indices puntos y plaquetas vecinos



int main(void )// Faltan las funciones que determina los índices de las plaquetas 
{
  FILE *input;
  FILE *output;
  FILE *pipe;
  
  char namein[255];
  char nameout[255];

  int i,j,k,l,f,b,dir;
  int n; // nº de elementos del bloque

  //OBSERVABLES
  float energia[NF];// Energia de curvatura
  float radio2g[NF];// Radio al cuadrado del giraton
  float caloresp; // Calor específico 

  //Promedios
  float media_radio2g;// media del radio del giraton al cuadrado
  float media_energia;// media del radio de la Energia de curvatura
  float media_energia2;

  //Valores acumulados
  float Energia[NF];// Energia de curvatura
  float Energia2[NF];// Energia de curvatura al cuadrado
  float Radio2g[NF];// Radio al cuadrado del giraton
  
  //Valores -bloque Jacknife
  float energiaJK[B];
  float energia2JK[B];
  float radio2gJK[B];
  float calorespJK;

  float sumJK_radio2g;
  float sumJK_energia;
  float sum2JK_energia;

  //Errores
  float error_radio2g;
  float error_energia;
  float error_caloresp;
  
  float sum_radio2g;//Radio de giraton, Suma de sus valores
  float sum_energia,sum2_energia; // Energia de curvatura, suma, suma de cuadrados
  
  // Cargamos los índices v[N][6]
  
  index_vcnos_prox();
  
  // Cargamos los índices p[N][6]
  
  for(i=0; i<N; i++)
    {
      for(dir=0; dir<6; dir++)
	{
	  p[i][dir]=index_plqta_prox(i,dir);
	}
    } 
  
  // Lectura de archivos de posicones y cálculo de 
  // la energía de curvatura  y el radio del giratón para cada uno

  if(NF%B!=0)
    {
      printf("El número de elementos del bloque no es entero\n");
      return 0;
    }
  
  for (k=0; k<B; k++)
    {
      radio2gJK[k]=0;
      energiaJK[k]=0; 
      energia2JK[k]=0; 
    }

  f=0;
  sprintf(namein,"xpos_L%d_K1.1-%d.dat",L,f);

  while((input=fopen(namein,"r"))!=NULL)
    {
      i=0;

      while(fscanf(input,"%f %f %f",&x[i].a,&x[i].b,&x[i].c)!=EOF)
	i++;
   
      if(i!=N)
	printf("Error: El fichero %s no contiene %d líneas\n",namein,N);      
      fclose(input);

      radio2g[f]=radio_giraton();
      
      Radio2g[NF-1]+=radio2g[f];
      Radio2g[f]=Radio2g[NF-1];

      energia[f]=energia_curvatura();
      Energia[NF-1]+=energia[f];
      Energia[f]=Energia[NF-1];

      Energia2[NF-1]+=(energia[f]*energia[f]);
      Energia2[f]=Energia2[NF-1];

      f++;
      sprintf(namein,"xpos_L%d_K1.1-%d.dat",L,f);
    }
  
  if(f!=NF)
    {
      printf("El número de archivos de posiciones leidos no es %d\n",NF);
      return 0;
    }

  // Promedios observables

  media_radio2g=Radio2g[NF-1]/(float)NF; //promedio radio giraton al cuadrdado
  media_energia=Energia[NF-1]/(float)NF; //promedio energia de curvatura
  caloresp=Energia2[NF-1]/(float)NF-Energia[NF-1]*Energia[NF-1]/(float)NF/(float)NF;
  caloresp*=(K*K)/((float)N); // promedio calor especifico

  // Errores de los observables en función del nº de bloques
  output=fopen("error_observables.dat","w"); 
  for(b=2; b<=NF; b++)
    {
      if((NF%b)==0)
	{
	  n=NF/b;

	  error_energia=0.0F;
	  error_caloresp=0;
	  error_radio2g=0.0F;

	  radio2gJK[0]=Radio2g[n-1];
	  energiaJK[0]=Energia[n-1];
	  energia2JK[0]=Energia2[n-1];

	  sumJK_radio2g=(Radio2g[NF-1]-radio2gJK[0])/(float)(NF-n);
	  error_radio2g+=pow(sumJK_radio2g-media_radio2g,2.0);
	  
	  sumJK_energia=(Energia[NF-1]-energiaJK[0])/(float)(NF-n);
	  error_energia+=pow(sumJK_energia-media_energia,2.0);
	  
	  sum2JK_energia=(Energia2[NF-1]-energia2JK[0])/(float)(NF-n);

	  calorespJK=sum2JK_energia-sumJK_energia*sumJK_energia;
	  calorespJK*=(K*K)/((float)N);
	  error_caloresp+=pow(caloresp-calorespJK,2.0);

	  for (k=1; k<b; k++)
	    {
	      radio2gJK[k]=Radio2g[(k+1)*n-1]-Radio2g[k*n-1];
	      energiaJK[k]=Energia[(k+1)*n-1]-Energia[k*n-1];
	      energia2JK[k]=Energia2[(k+1)*n-1]-Energia2[k*n-1];

	      sumJK_radio2g=(Radio2g[NF-1]-radio2gJK[k])/(float)(NF-n);
	      error_radio2g+=pow(sumJK_radio2g-media_radio2g,2.0);

	      sumJK_energia=(Energia[NF-1]-energiaJK[k])/(float)(NF-n);
	      error_energia+=pow(sumJK_energia-media_energia,2.0);

	      sum2JK_energia=(Energia2[NF-1]-energia2JK[k])/(float)(NF-n);

	      calorespJK=sum2JK_energia-sumJK_energia*sumJK_energia;
	      calorespJK*=(K*K)/((float)N);
	      error_caloresp+=pow(caloresp-calorespJK,2.0);
	    }

	  error_radio2g=(float)(b-1)/((float) b) * error_radio2g;
	  error_radio2g=sqrt(error_radio2g);

	  error_energia=(float)(b-1)/((float) b) * error_energia;
	  error_energia=sqrt(error_energia);

	  error_caloresp=(float)(b-1.)/((float) b) * error_caloresp;
	  error_caloresp=sqrt(error_caloresp);
	  fprintf(output,"%d %f %f %f\n",n ,error_energia,error_caloresp,error_radio2g);  
	}
    }

  //GRAFICA

  pipe = popen("gnuplot -persist","w");  
  fprintf(pipe, "set logscale x\n");
  fprintf(pipe, "set multiplot layout 3,1 \n");
  fprintf(pipe, "plot \"./error_observables.dat\" u 1:2 title \"energia\" w lp\n");
  fprintf(pipe, "plot \"./error_observables.dat\" u 1:3 title \"Cv\" w lp\n");
  fprintf(pipe, "plot \"./error_observables.dat\" u 1:4 title \"Radio2 g\" w lp\n");
  fprintf(pipe, "unset multiplot"); 
  fflush(pipe);
  close(pipe);

  return 1;
}

float energia_curvatura(void)
{
  
  int i,j,k;
  int s1,s2;
  float energia;
  
  vector r1,r2,r3,ntemp,n[M];
  float norma2,invnorma;
 
  energia=0.0F;

  for(s1=1; s1<L; s1++)
    {
      for(s2=0; s2<L-1; s2++)
	{

	  i=s1+L*s2;

	  //Vectores relativos:

	  _resta(r1,x[v[i][1]],x[i]);
	  _resta(r2,x[v[i][2]],x[i]);
	  _resta(r3,x[v[i][3]],x[i]);

	  //Productos vectoriales:

	   _prodvec(ntemp,r1,r2);
	   norma2=_norma2(ntemp);
	   invnorma=1.0F/sqrt(norma2);
	   _escala(ntemp,invnorma);
	   n[p[i][1]]=ntemp;

	   _prodvec(ntemp,r2,r3);
	   norma2=_norma2(ntemp);
	   invnorma=1.0F/sqrt(norma2);
	   _escala(ntemp,invnorma);
	   n[p[i][2]]=ntemp;
	}
    }

  // Productos dirección 1

  for(s1=1; s1<L-1; s1++)
    {
      for(s2=0; s2<L-1; s2++)
	{
	  i=s1+L*s2;
	  energia+=_prodesc(n[p[i][1]],n[p[i][0]]);
	}      
    }
  // Productos dirección 2

  for(s1=0; s1<L; s1++)
    {
      for(s2=0; s2<L-1; s2++)
	{
	  i=s1+L*s2;
	  energia+=_prodesc(n[p[i][2]],n[p[i][1]]);
	}      
    }
  // Productos dirección 3

  for(s1=1; s1<L; s1++)
    {
      for(s2=1; s2<L-1; s2++)
	{
	  i=s1+L*s2;
	  energia+=_prodesc(n[p[i][3]],n[p[i][2]]);
	}      
    }

    return energia;
}

void index_vcnos_prox()
{
  int s1,s2;
  int s1min,s1max,s2min,s2max;
  int dir;
  int s1_dir,s2_dir,s1_dir_old,s2_dir_old;
  int i;

  for(s1_dir=1,s2_dir=0,dir=0; dir<6; dir++)
    {
      switch(s1_dir)
	{
	case -1:
	  s1min=1;
	  s1max=L;
	  break;
	case 0:
	  s1min=0;
	  s1max=L;
	  break;
	case 1:
	  s1min=0;
	  s1max=L-1;
	  break;
	}

      switch(s2_dir)
	{
	case -1:
	  s2min=1;
	  s2max=L;
	  break;
	case 0:
	  s2min=0;
	  s2max=L;
	  break;
	case 1:
	  s2min=0;
	  s2max=L-1;
	  break;
	}

      for(s2=0; s2<L; s2++)
	{
	for(s1=0; s1<L; s1++)
	  {
	    i=s1+L*s2;
	    if(s1>=s1min && s1<s1max && s2>=s2min && s2<s2max)
	      v[i][dir] = i + s1_dir + L*s2_dir;
	    else
	      v[i][dir]=-1;
	  }
	}

      s1_dir_old=s1_dir;
      s2_dir_old=s2_dir;

      s1_dir=-s2_dir_old;
      s2_dir=s1_dir_old + s2_dir_old;
    }
}

/*INDEX_PLQTA_DIR1(int i):
*  Devuelve el índice de la plaqueta vecina del nodo i en la dirección 1
*   * Las direcciones de las plaquetas vecinas se muestran en la figura:
*
* 		           x-----x
* 		          / \ 1 / \
* 		         / 2 \ / 0 \
* 		        x-----i-----x
* 		         \ 3 / \ 5 /
* 		          \ / 4 \ /
* 		           x-----x
*
*/
int index_plqta_dir1(int i)
{
  int p;

  if(v[i][1]!=-1 && v[i][2]!=-1)
    p=2*i - (2* (i/L) + 1);
  else
    {
      p=-1;
    }
return p;
}

/*INDEX_PLQTA_DIR0(int i):
*  Devuelve el índice de la plaqueta vecina del nodo i en la dirección 0
*   * Las direcciones de las plaquetas vecinas se muestran en la figura:
*
* 		           x-----x
* 		          / \ 1 / \
* 		         / 2 \ / 0 \
* 		        x-----i-----x
* 		         \ 3 / \ 5 /
* 		          \ / 4 \ /
* 		           x-----x
*
*/
int index_plqta_dir0(int i)
{
  int p;

  if(v[i][0]!=-1 && v[i][1]!=-1)
    p=2*i - (2* (i/L));
  else
    {
      p=-1;
    }
  return p;
}


/*INDEX_PLQTA_PROX(int i int dir):
*  Devuelve el índice de la plaqueta vecina del nodo i en la dirección dir
*   * Las direcciones de las plaquetas vecinas se muestran en la figura:
*
* 		           x-----x
* 		          / \ 1 / \
* 		         / 2 \ / 0 \
* 		        x-----i-----x
* 		         \ 3 / \ 5 /
* 		          \ / 4 \ /
* 		           x-----x
*
*/
int index_plqta_prox(int i, int dir)
{
  int p;

  switch(dir)
    {
    case 0:
      p=index_plqta_dir0(i);
      break;
    case 1:
      p=index_plqta_dir1(i);
      break;
    case 2:
      p=index_plqta_dir0(v[i][3]);
      break;
    case 3:
      p=index_plqta_dir1(v[i][4]);
      break;
    case 4:
      p=index_plqta_dir0(v[i][4]);
      break;
    case 5:
      p=index_plqta_dir1(v[i][5]);
      break;
    }
  return p;
}
float radio_giraton(void )
{
  int i;
  float sum_norma2,norma2,norma2sumx,norma2xcm;
  float densidad,momento2,r2giraton;
  vector sum_x,xcm;
  

  sum_norma2=0.0F; 

  sum_x.a=0.0F;
  sum_x.b=0.0F;  
  sum_x.c=0.0F;

  norma2=0.0F;

  for(i=0; i<N; i++)
    {
      norma2=_norma2(x[i]);
      _suma(sum_x,sum_x,x[i]);
      sum_norma2+=norma2;      
    }
  
  densidad=1.0F/ ((float) N);
  
  xcm=sum_x;
  _escala(xcm,densidad);   
  norma2xcm=_norma2(xcm);
  momento2=sum_norma2/ ((float) N);

    return r2giraton=(momento2-norma2xcm)/3.0F;
}

