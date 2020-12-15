// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// archivo: prodcons_1.cpp
// Ejemplo de un monitor en C++11 con semántica SC, para el problema
// del productor/consumidor, con un único productor y un único consumidor.
// Opcion LIFO (stack)
//
// Historial:
// Creado en Julio de 2017
// -----------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;

constexpr int
   num_items  = 160 ;     // número de items a producir/consumir

const int hebras_productoras = 4, hebras_consumidoras = 4,datos_por_consumidor = num_items/hebras_consumidoras,datos_por_productor = num_items/hebras_productoras;

mutex
   mtx ;                 // mutex de escritura en pantalla
unsigned
   cont_prod[num_items], // contadores de verificación: producidos
   cont_cons[num_items], // contadores de verificación: consumidos
   producidos[hebras_productoras]={0},
   consumidos[hebras_consumidoras]={0};

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

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato(int indice)
{
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "producido: " << contador << endl << flush ;
   mtx.unlock();
   producidos[indice]++;
   cont_prod[contador] ++ ;
   return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   if ( num_items <= dato )
   {
      cout << " dato === " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
   }
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "                  consumido: " << dato << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void ini_contadores()
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }

   for(int i = 0 ; i < hebras_productoras ; i++){
       if(producidos[i] != datos_por_productor){
           ok = false;
           cout<<"Cada hebra no ha producido el numero correcto de elementos"<<endl;
       }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SC, un prod. y un cons.

class ProdCons1SU : public HoareMonitor
{
 private:
 static const int           // constantes:
   num_celdas_total = 40;   //  núm. de entradas del buffer
 int                        // variables permanentes
   buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
   primera_libre ;          //  indice de celda de la próxima inserción
 mutex
   cerrojo_monitor ;        // cerrojo del monitor
 CondVar        // colas condicion:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres ;                 //  cola donde espera el productor  (n<num_celdas_total)

 public:                    // constructor y métodos públicos
   ProdCons1SU(  ) ;           // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

ProdCons1SU::ProdCons1SU(  )
{
   primera_libre = 0 ;
   ocupadas = newCondVar();
   libres   = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdCons1SU::leer(  )
{

   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   if ( primera_libre == 0 )
      ocupadas.wait( );

   // hacer la operación de lectura, actualizando estado del monitor
   assert( 0 < primera_libre  );
   primera_libre-- ;
   const int valor = buffer[primera_libre] ;


   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();

   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------
/*
SC : Hay que tener en cuenta que ahora varias hebras esperan en el wait(), si por ejemplo
tenemos varios consumidores esperando en el wait con el if, puede que otra hebra consumidora entre justo 
después de producir un valor, gane el cerrojo y lo consuma. Si tuvieramos el if, la hebra que espera iría
a consumir inmediatamente después de esta y produciría una aserción,por lo que con el while vuelve a comprobar
la condición para ver si alguna hebra consumidora se le ha "colado" y así seguir esperando

SU : Con esta semántica si funciona la condición con if ya que al hacer signal, la hebra que coge la guarda
es la primera que ha hecho el wait, por lo que coge la guarda y hace que una que entre al mismo tiempo que se escribe,
le toque "esperar en el wait ya que ha decrementado el valor de primera_libre"
*/
void ProdCons1SU::escribir( int valor )
{

   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   if ( primera_libre == num_celdas_total )
      libres.wait();

   //cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
   assert( primera_libre < num_celdas_total );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[primera_libre] = valor ;
   primera_libre++ ;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<ProdCons1SU>  monitor , int indice)
{
   for( unsigned i = 0 ; i < num_items/hebras_productoras ; i++ )
   {
      int valor = producir_dato(indice) ;
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdCons1SU> monitor)
{
   for( unsigned i = 0 ; i < num_items/hebras_consumidoras ; i++ )
   {
      int valor = monitor->leer();
      consumir_dato( valor ) ;
   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (1 prod/cons, Monitor SU, buffer LIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<ProdCons1SU> monitor = Create<ProdCons1SU>();

   

   thread hebra_productora[hebras_productoras], 
          hebra_consumidora[hebras_consumidoras];

          for(int i = 0 ; i < hebras_productoras ; i++){
              hebra_productora[i] = thread( funcion_hebra_productora, monitor ,i);
          }

          for(int i = 0 ; i < hebras_consumidoras ; i++){
              hebra_consumidora[i] = thread(funcion_hebra_consumidora, monitor);
          }

          for(int i = 0 ; i < hebras_productoras ; i++){
              hebra_productora[i].join();
          }

          for(int i = 0 ; i < hebras_consumidoras ; i++){
              hebra_consumidora[i].join();
          }


   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}