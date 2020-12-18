// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: filosofos-cam.cpp
// Implementación del problema de los filósofos (con camarero).
// Plantilla para completar.
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------


#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   num_filosofos = 5 ,
   num_procesos  = 2*num_filosofos + 1,
   id_camarero   = 10 ,
   max_filosofos_en_mesa = 4 ,
   etiq_tenedor    = 0,
   etiq_sentarse = 1,
   etiq_levantarse = 2;

/*
Para realizar este problema, se implementa una nueva función , la del camarero,
que solo la ejecutará un proceso. En esta función, el camarero hace lo siguiente:
Sienta al máximo de filósofos posibles (todos menos uno), y a continuación levanta 
a cada filósofo conforme estos lo van pidiendo.Siempre que se levanta a un filósofo, el
camarero rellena su hueco con otro filósofo que quiera sentarse. Al ser el envío síncrono,
el camarero debe esperar a que un filósofo solicite sentarse para ver si puede levantar al 
siguiente

La función que realiza los filósofos es la siguiente, Manda una señal sincrona para solicitar
sentarse, a continuación coge los tenedores , come y los suelta, y después de esto, el filósofo
solicita levantarse al camarero, para ponerse a pensar

Para este problema, los procesos tenedor y camarero pueden recibir distintas señales(sentarse, levantarse,
coger el tenedor y soltarlo), por lo que es necesario diferenciarlas con etiquetas para que cada MPI_Recv reciba
las señales adecuadas y no se solapen, dando lugar a errores
*/

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

// ---------------------------------------------------------------------

void funcion_filosofos( int id )
{
  int id_ten_izq = (id+1)              % (num_procesos - 1), //id. tenedor izq.
      id_ten_der = (id+num_procesos-2) % (num_procesos - 1); //id. tenedor der.

  while ( true )
  {
    //Se sienta el filósofo
    cout <<"Filósofo " << id << " solicita sentarse"<<endl;
    MPI_Ssend(&id , 1 , MPI_INT , id_camarero , etiq_sentarse , MPI_COMM_WORLD);
    
    //Coge el tenedor izquierdo
    cout <<"Filósofo " <<id << " solicita ten. izq." <<id_ten_izq <<endl;
    MPI_Ssend(&id , 1 , MPI_INT , id_ten_izq , etiq_tenedor , MPI_COMM_WORLD );

    //Coge el tenedor derecho
    cout <<"Filósofo " <<id <<" solicita ten. der." <<id_ten_der <<endl;
    MPI_Ssend(&id , 1 , MPI_INT , id_ten_der , etiq_tenedor , MPI_COMM_WORLD ); 

    //Come
    cout <<"Filósofo " <<id <<" comienza a comer" <<endl ;
    sleep_for( milliseconds( aleatorio<10,100>() ) );

    //Suelta el tenedor izquierdo
    cout <<"Filósofo " <<id <<" suelta ten. izq. " <<id_ten_izq <<endl;
    MPI_Ssend(&id , 1 , MPI_INT , id_ten_izq , etiq_tenedor , MPI_COMM_WORLD );

    //Suelta el tenedor derecho
    cout << "Filósofo " <<id <<" suelta ten. der. " <<id_ten_der <<endl;
    MPI_Ssend(&id , 1 , MPI_INT , id_ten_der , etiq_tenedor , MPI_COMM_WORLD );

    //Se levanta
    cout << "Filósofo " << id << " solicita levantarse " << endl;
    MPI_Ssend(&id , 1 , MPI_INT , id_camarero , etiq_levantarse , MPI_COMM_WORLD );
 
    //Piensa
    cout << "Filosofo " << id << " comienza a pensar" << endl;
    sleep_for( milliseconds( aleatorio<10,100>() ) );
 }
}
// ---------------------------------------------------------------------


/*
Puede ocurrir interbloqueo cuando todos los filósofos cogen el tenedor izquierdo 
y se quedan esperando al derecho
*/
void funcion_tenedores( int id )
{
  int valor, id_filosofo ;  // valor recibido, identificador del filósofo
  MPI_Status estado ;       // metadatos de las dos recepciones

  while ( true )
  {
    
     MPI_Recv ( &id_filosofo , 1 , MPI_INT , MPI_ANY_SOURCE , etiq_tenedor , MPI_COMM_WORLD , &estado );
     cout <<"Ten. " <<id <<" ha sido cogido por filo. " <<id_filosofo <<endl;

     
     MPI_Recv ( &valor , 1 , MPI_INT , id_filosofo , etiq_tenedor , MPI_COMM_WORLD , &estado );
     cout <<"Ten. "<< id<< " ha sido liberado por filo. " <<id_filosofo <<endl ;
  }
}
// ---------------------------------------------------------------------

void funcion_camarero(){
    int id_sentado,
        id_levantado,
        filosofos_sentados = 0;
    MPI_Status estado ;
    
    while ( true )
    {
        //Se sientan el máximo posible de filósofos a la mesa
        while (filosofos_sentados < max_filosofos_en_mesa){
            MPI_Recv( &id_sentado , 1 , MPI_INT , MPI_ANY_SOURCE , etiq_sentarse , MPI_COMM_WORLD , &estado );
            cout << "Se ha sentado el filósofo "<<id_sentado<<" a la mesa"<<endl;
            filosofos_sentados++;
        }

        //Cuando se levanta uno, se vuelve a buscar sitio para que se siente otro
        MPI_Recv( &id_levantado , 1 , MPI_INT , MPI_ANY_SOURCE , etiq_levantarse , MPI_COMM_WORLD , &estado );
        cout<<"Se ha levantado el filósofo "<<id_levantado<<" de la mesa"<<endl;
        filosofos_sentados--;
    }
    
}

// ---------------------------------------------------------------------


int main( int argc, char** argv )
{
   int id_propio, num_procesos_actual ;

   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


   if ( num_procesos == num_procesos_actual )
   {
      // ejecutar la función correspondiente a 'id_propio'
        if ( id_propio == id_camarero )
            funcion_camarero();
        else{
            if(id_propio % 2 == 0)
                funcion_filosofos( id_propio );
            else
                funcion_tenedores( id_propio );
            
        } 
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   MPI_Finalize( );
   return 0;
}

// ---------------------------------------------------------------------
