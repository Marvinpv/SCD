// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: examenp3.cpp
// Implementación del problema de las cajas.
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
    num_clientes = 10,
    num_procesos = num_clientes + 1,
    id_intermediario = num_clientes,
    etiq_pagar   = 0,
    etiq_salir   = 1,
    etiq_inter   = 2,
    cajas        = 3;



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


void funcion_cliente(int id_cliente){

    int num_caja;

    MPI_Status estado ;       // metadatos de las dos recepciones

    while(true){
        cout<<"Cliente "<<id_cliente<<" comienza a comprar"<<endl;
        sleep_for( milliseconds( aleatorio<10,100>() ) );

        cout<<"Cliente "<<id_cliente<<" solicita pagar"<<endl;
        MPI_Ssend(&id_cliente, 1 , MPI_INT , id_intermediario, etiq_pagar , MPI_COMM_WORLD);
        MPI_Recv(&num_caja, 1 , MPI_INT , id_intermediario , etiq_inter , MPI_COMM_WORLD, &estado);

        cout<<"Cliente "<<id_cliente<< " entra a pagar en caja "<<endl;
        sleep_for( milliseconds( aleatorio<10,100>() ) );

        
        MPI_Ssend(&id_cliente , 1 , MPI_INT , id_intermediario, etiq_salir, MPI_COMM_WORLD);
        cout<<"Cliente "<<id_cliente<<" ha terminado la compra"<<endl;
        sleep_for( milliseconds( aleatorio<10,100>() ) );
    }

    
}

void funcion_intermediario(){
    int cajas_ocupadas = 0,
        cliente;
    
    MPI_Status estado ;       // metadatos de las dos recepciones

    while(true){
        if(cajas_ocupadas < cajas){
            MPI_Recv(&cliente , 1 , MPI_INT , MPI_ANY_SOURCE , MPI_ANY_TAG ,MPI_COMM_WORLD, &estado );
        }else{
            MPI_Recv(&cliente, 1 , MPI_INT , MPI_ANY_SOURCE, etiq_salir , MPI_COMM_WORLD , &estado);
        }

        if(estado.MPI_TAG == etiq_pagar){
            MPI_Ssend(&cajas_ocupadas, 1 , MPI_INT , estado.MPI_SOURCE , etiq_inter , MPI_COMM_WORLD);
            cout<<"Cliente "<<cliente<<" ha pagado en caja"<<endl;
            cajas_ocupadas++;
            cout<<"Cajas ocupadas "<<cajas_ocupadas<<endl;
        }else if(estado.MPI_TAG == etiq_salir){
            cout<<"Cliente "<<cliente<<" sale de caja"<<endl;
            cajas_ocupadas--;
            cout<<"Cajas ocupadas "<<cajas_ocupadas<<endl;
        }
        
    }
    
}



int main( int argc, char** argv )
{
   int id_propio, num_procesos_actual ;

   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


   if ( num_procesos == num_procesos_actual )
   {
      // ejecutar la función correspondiente a 'id_propio'
        if ( id_propio < num_clientes )
            funcion_cliente(id_propio);
        else{
            funcion_intermediario();
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