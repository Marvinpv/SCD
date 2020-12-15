#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;




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

const int num_fumadores = 3;       //Número de fumadores del programa

class Estanco : public HoareMonitor
{
 private:
 

 CondVar                                      // colas condicion:
   mesa_libre,                                //  cola donde espera el consumidor (n>0)
   producido[num_fumadores] ;                 //  cola donde espera el productor  (n<num_celdas_total)

 bool
   mesa_ocupada;
 int    
   ing_puesto;

 public:                    // constructor y métodos públicos
   Estanco(  ) ;                 // constructor
   void obtenerIngrediente(int ing);
   void esperarRecogidaIngrediente(int ing);          // acción de fumar (fumador)
   void poner_ingrediente(int ing);    // generar un producto(estanquero)
} ;
// -----------------------------------------------------------------------------

Estanco::Estanco(  )
{
    mesa_ocupada = false;
    mesa_libre = newCondVar();
    ing_puesto = -1;
    for(int i = 0 ; i < num_fumadores ; i++){
        producido[i] = newCondVar();
    }
}

void Estanco::poner_ingrediente(int ing){
    mesa_ocupada = true;
    cout<<"Estanquero : pone el ingrediente "<<ing<<" en la mesa"<<endl;
    producido[ing].signal();
    ing_puesto = ing;
}

void Estanco::esperarRecogidaIngrediente(int ing){
    if(mesa_ocupada)
        mesa_libre.wait();

    ing_puesto = -1;
}

void Estanco::obtenerIngrediente(int ing){
    if(ing_puesto != ing)
        producido[ing].wait();
    
    mesa_libre.signal();
    mesa_ocupada = false;
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

    return num_ingrediente ;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero( MRef<Estanco> monitor )
{
    while(true){
        int ingrediente = producir_ingrediente();
        monitor->poner_ingrediente(ingrediente);
        monitor->esperarRecogidaIngrediente(ingrediente);
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
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos), La mesa se queda libre" << endl;
    

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador(MRef<Estanco> monitor, int num_fumador )
{
    while(true){
        monitor->obtenerIngrediente(num_fumador);
        fumar(num_fumador);
    }
}

//----------------------------------------------------------------------

int main()
{
   
   
   
   cout << "--------------------------------------------------------" << endl
        << "               Problema de los fumadores                " << endl
        << "--------------------------------------------------------" << endl
        << flush ;
    

    MRef<Estanco> monitor = Create<Estanco>();

   thread estanquero( funcion_hebra_estanquero, monitor ),fumadores[num_fumadores];
   for(int i = 0 ;i < num_fumadores ; i++){
      fumadores[i] = thread(funcion_hebra_fumador,monitor,i);
   }

   estanquero.join();
   for(int i = 0 ;i < num_fumadores ; i++){
      fumadores[i].join();
   }

}