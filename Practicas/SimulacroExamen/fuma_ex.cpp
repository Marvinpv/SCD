/**
 * @file fuma_ex.cpp
 * @author Marvin Peinado Vidaña
 * @brief Problema de los fumadores para examen de simulación de SCD
 * En este Problema, 2 fumadores necesitan del mismo ingrediente y se 
 * turnan para conseguirlo
 * @version 0.1
 * @date 2020-11-10
 * 
 * @copyright Copyright (c) 2020
 * 
 */


#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

const int num_fumadores = 4;  //Número de fumadores del programa, los dos primeros fumadores esperan el mismo elemento y se turnan
                              //para conseguirlo

Semaphore mesa_libre = 1;     //Semáforo para indicar que la mesa está libre
Semaphore producido[4] = {0,0,0,0}; //Semáforo para indicar que un fumador está listo para fumar


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

int turno = aleatorio<0,1>(); //Variable que contea el turno de los dos fumadores que necesitan cerillas

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

   const int num_ingrediente = aleatorio<0,num_fumadores-2>() ;

   // informa de que ha terminado de producir
   cout << "Estanquero : termina de producir ingrediente " << num_ingrediente + 1<< endl; //Para que cada fumador fume su num de ingrediente 
                                                                                          //y los fumadores 0 y 1 se repartan el ing 1
                                                                                          //Se trata de una cuestion de estética de lo que se ve en pantalla

   return num_ingrediente ;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
   int i;
      while( true ){

         mesa_libre.sem_wait();
         i = producir_ingrediente();
         if(i == 0){                //Si el ingrediente producido son cerillas 
            producido[ turno ].sem_signal(); //Dependiendo del turno se elige al fumador 0 o 1 y luego 
            turno = (turno + 1) % 2;         //Se cambia de turno
         }else{
            producido[i + 1].sem_signal();    //Si no son cerillas el programa funciona con normalidad,
         }                                    //Teniendo en cuenta que hay un fumador mas, por lo que
                                              //el ing 2 es para el fumador 3 y el 1 para el 2(Esto se corrige en la
                                              //salida por pantalla para no generar confusión)
         
      }

}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
// La hebra fumador funciona normal, es la del estanquero la
// que decide a quien darle el ingrediente
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

int main()
{
   
   
   
   cout << "--------------------------------------------------------" << endl
        << "               Problema de examen de los fumadores      " << endl
        << "  Los fumadores 0 y 1 necesitan del ingrediente 1 para  " << endl
        << "           fumar, y se turnan para conseguirlo          " << endl
        << "--------------------------------------------------------" << endl
        << flush ;

   thread estanquero( funcion_hebra_estanquero ),fumadores[num_fumadores];
   for(int i = 0 ;i < num_fumadores ; i++){
      fumadores[i] = thread(funcion_hebra_fumador,i);
   }

   estanquero.join();
   for(int i = 0 ;i < num_fumadores ; i++){
      fumadores[i].join();
   }

}