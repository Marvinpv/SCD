#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

const int num_fumadores = 3,        //Número de fumadores del programa
            gritos_activista = 8;   //Gritos para que el activista se tumbe en el suelo y luego comienze a gritar;
Semaphore mesa_libre = 1;           //Semáforo para indicar que la mesa está libre
Semaphore producido[3] = {0,0,0};   //Semáforo para indicar que un fumador está listo para fumar
Semaphore robado = 0;
Semaphore mensajes = 1;
int gritos_producidos = 1;
bool activista_tumbado = false;



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

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   
   cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;
    
   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   mensajes.sem_wait();
   cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;
    mensajes.sem_signal();
   return num_ingrediente ;
}

//----------------------------------------------------------------------

//Función que simula el robo del ingrediente
void robar(int ing){
    mensajes.sem_wait();
    cout<<"Ingrediente número "<<ing<<" robado por el activista..."<<endl;
    mensajes.sem_signal();
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
   int i;
      while( true ){

         mesa_libre.sem_wait();
         i = producir_ingrediente();

         if(activista_tumbado){
            robar(i);
            robado.sem_signal();
           activista_tumbado = false;

         }else{
            producido[i].sem_signal();
         }
        
         
      }

}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar
    mensajes.sem_wait();
    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
    mensajes.sem_signal();
   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   while( true )
   {
      producido[num_fumador].sem_wait();
      cout << "Fumador "<< num_fumador << "  : recoge su ingrediente y comienza a liarse el cigarro"<<endl;
      mesa_libre.sem_signal();
      fumar(num_fumador);

   }
}

//----------------------------------------------------------------------

//Función que simula el grito del activista
void gritar(){
    chrono::milliseconds duracion_gritar( aleatorio<20,200>() );
    mensajes.sem_wait();
    cout<<"¡FUMAR MATA!"<<endl;
    mensajes.sem_signal();

    this_thread::sleep_for( duracion_gritar );

        

}


//Funcion que ejecuta la hebra del activista
void funcion_hebra_activista(){
    while(true){
        gritar();

        if(gritos_producidos == gritos_activista){
            cout<<"El activista se tumba por que ya ha gritado varias veces..."<<endl;
            activista_tumbado = true;

            robado.sem_wait();
            gritos_producidos = 1;
            mesa_libre.sem_signal();
        }else{
            gritos_producidos++;
        }
    }
}


//----------------------------------------------------------------------

int main()
{
   
   
   
   cout << "--------------------------------------------------------" << endl
        << "               Problema de los fumadores                " << endl
        << "--------------------------------------------------------" << endl
        << flush ;

   thread activista( funcion_hebra_activista );
   thread estanquero( funcion_hebra_estanquero ),fumadores[num_fumadores];
   for(int i = 0 ;i < num_fumadores ; i++){
      fumadores[i] = thread(funcion_hebra_fumador,i);
   }

   activista.join();
   estanquero.join();
   for(int i = 0 ;i < num_fumadores ; i++){
      fumadores[i].join();
   }

}