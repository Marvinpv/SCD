/**
 * @file prodcons_ex.cpp
 * @author Marvin Peinado Vidaña
 * @brief Problema de los productores consumidores para la simulación
 * de examen. En este problema los productores pares generan números en un 
 * buffer distinto al de los impares. Los lectores acceden a los dos buffers,
 * y, si los dos están libres, se tiene prioridad sobre el bufer de impares
 * @version 0.1
 * @date 2020-11-10
 * 
 * @copyright Copyright (c) 2020
 * 
 */

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

class ProdCons_EX : public HoareMonitor
{
 private:
 static const int           // constantes:
   num_celdas_total = 40;   //  núm. de entradas del buffer
 int                        // variables permanentes
   buffer_impares[num_celdas_total/2],//  buffer de tamaño fijo, con los datos : v1
   buffer_pares[num_celdas_total/2],  //  v2
   primera_libre_impares,          //  indice de celda de la próxima inserción
   primera_libre_pares;

 CondVar        // colas condicion:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres_pares,                 //  cola donde espera el productor  (n<num_celdas_total)
   libres_impares;

 public:                    // constructor y métodos públicos
   ProdCons_EX(  ) ;           // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir_pares( int valor ); // insertar un valor (sentencia E) en el vector de pares(productor)
   void escribir_impares( int valor ); // insertar un valor (sentencia E) en el vector de impares(productor)
} ;
// -----------------------------------------------------------------------------

ProdCons_EX::ProdCons_EX(  )
{
   primera_libre_impares = 0 ;
   primera_libre_pares   = 0 ;
   ocupadas = newCondVar();
   libres_pares   = newCondVar();
   libres_impares = newCondVar();
}

// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdCons_EX::leer(  )
{

    int valor;

   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   if ( primera_libre_pares == 0 && primera_libre_impares == 0)
            ocupadas.wait();

   if(primera_libre_impares != 0){
       // hacer la operación de lectura, actualizando estado del monitor
            assert( 0 < primera_libre_impares  );
            primera_libre_impares-- ;
            valor = buffer_impares[primera_libre_impares] ;
            // señalar al productor que hay un hueco libre, por si está esperando
            libres_impares.signal();

   }else if(primera_libre_pares != 0){
       // hacer la operación de lectura, actualizando estado del monitor
            assert( 0 < primera_libre_pares );
            primera_libre_pares-- ;
            valor = buffer_pares[primera_libre_pares] ;
            // señalar al productor que hay un hueco libre, por si está esperando
            libres_pares.signal();

   }         
        
   

   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void ProdCons_EX::escribir_pares( int valor )
{

   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   if ( primera_libre_pares == num_celdas_total/2 )
      libres_pares.wait();

   //cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
   assert( primera_libre_pares < num_celdas_total/2 );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer_pares[primera_libre_pares] = valor ;
   primera_libre_pares++ ;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}

void ProdCons_EX::escribir_impares( int valor )
{

   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   if ( primera_libre_impares == num_celdas_total/2 )
      libres_impares.wait();

   //cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
   assert( primera_libre_impares < num_celdas_total/2 );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer_impares[primera_libre_impares] = valor ;
   primera_libre_impares++ ;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}




// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora_par( MRef<ProdCons_EX>  monitor , int indice)
{
   for( unsigned i = 0 ; i < num_items/hebras_productoras ; i++ )
   {
      int valor = producir_dato(indice) ;
      monitor->escribir_pares( valor );
   }
}

void funcion_hebra_productora_impar( MRef<ProdCons_EX>  monitor , int indice)
{
   for( unsigned i = 0 ; i < num_items/hebras_productoras ; i++ )
   {
      int valor = producir_dato(indice) ;
      monitor->escribir_impares( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdCons_EX> monitor)
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
        << "Problema de los productores-consumidores de examen ( Monitor SU, buffer LIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<ProdCons_EX> monitor = Create<ProdCons_EX>();

   

   thread hebra_productora[hebras_productoras], 
          hebra_consumidora[hebras_consumidoras];

          for(int i = 0 ; i < hebras_productoras ; i++){
              if(i % 2 == 0){
                  hebra_productora[i] = thread( funcion_hebra_productora_par, monitor ,i);
              }else{
                  hebra_productora[i] = thread( funcion_hebra_productora_impar, monitor ,i);
              }
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