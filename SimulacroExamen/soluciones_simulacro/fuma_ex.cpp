#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

Semaphore disponibles[4] = {0, 0, 0, 0};  //el cuarto fumador necesita el mismo ingrediente que el tercero (2)
Semaphore mostr_vacio = 1;  //semaforo del estanquero
int ultima_vez;  //para saber cuál es el fumador (tercero o cuarto) que fumó la última vez
bool primera_vez = true;  //para decidir que fumador fuma la primera vez que se ponga el ingrediente 2
Semaphore mensajes = 1;  //semaforo de exclusion mutua para que no se solapen los mensajes en pantalla


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

int Producir(){
   int producido = aleatorio <0, 2>();

   return producido;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{

  while (true){
     int ingrediente = Producir();
     sem_wait (mostr_vacio);
     sem_wait(mensajes);
     cout << "El estanquero ha puesto el ingrediente " << ingrediente << " en el mostrador" << endl;
     sem_signal(mensajes);
    
     if (ingrediente == 2)
     {    
        if (primera_vez)
           {
             ingrediente = aleatorio <2, 3>();
             primera_vez = false;
           }
        else{
           if (ultima_vez == 2)
              ingrediente = 3;
           else
              ingrediente = 2;           
           }
        ultima_vez = ingrediente;
     }

        sem_signal (disponibles[ingrediente]);     
  }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar
     sem_wait(mensajes);
     cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
     sem_signal(mensajes);

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar
     sem_wait(mensajes);
     cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
     sem_signal(mensajes);

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   while( true )
   {
     sem_wait(mensajes);
     cout << "El fumador " << num_fumador << " espera el suministro " << num_fumador << endl;
     sem_signal(mensajes);
     sem_wait (disponibles[num_fumador]);

     sem_wait(mensajes);
     cout << "El fumador " << num_fumador << " retira su ingrediente." << endl;
     sem_signal(mensajes);
     sem_signal (mostr_vacio);
     fumar (num_fumador);

   }
}

//----------------------------------------------------------------------

int main()
{
   // declarar hebras y ponerlas en marcha
   // ......

   cout << "\nLos fumadores 2 y 3 son los que necesitan cerillas para fumar.\n\n";

   thread hebra_estanquero (funcion_hebra_estanquero);
   thread hebra_fumador[4];
   thread hebra_cuarto_fumador;

   for (int i = 0; i < 4; i++){
      hebra_fumador[i] = thread (funcion_hebra_fumador, i);
   }

   hebra_estanquero.join();
   
   for (int i = 0; i < 4; i++){
      hebra_fumador[i].join();
   }
}
