
#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include "HoareMonitor.h"
using namespace std;
using namespace HM;

constexpr int
   num_items  = 40 ;     // número de items a producir/consumir

mutex
   mtx ;                 // mutex de escritura en pantalla
unsigned
   cont_prod[num_items], // contadores de verificación: producidos
   cont_cons[num_items]; // contadores de verificación: consumidos
const int numero_productores=8; //numero de hebras productoras
const int numero_consumidores=5; //numero de hebras consumidoras

int valores_por_hebra[numero_productores]={0};
int producir_por_hebra = num_items/numero_productores;  //numero de valores que produce cada hebra productora
int consumir_por_hebra = num_items/numero_consumidores;  //numero de valores que consume cada hebra consumidora
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

int producir_dato(int numero_hebra)
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   int producido = numero_hebra*producir_por_hebra+valores_por_hebra[numero_hebra];
   cout << "productor: " << numero_hebra <<" \tproducido: " << producido << endl << flush ;
   mtx.unlock();
   cont_prod[producido] ++ ;
   valores_por_hebra[numero_hebra]++; //aumentamos en uno los valores producidos por la hebra
   return producido ;
}
//----------------------------------------------------------------------

void consumir_dato( int numero_hebra, unsigned dato )
{
   if ( num_items <= dato )
   {
      cout << " dato === " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
   }
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "consumidor: " << numero_hebra << "            \t\tconsumido: " << dato << endl ;
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
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SC, un prod. y un cons.

class ProdCons2SU : public HoareMonitor  //definimos la clase como derivada de tipo public
{
 private:
 static const int           // constantes:
   num_celdas_pares = 5,     //  núm. de entradas del buffer v2
   num_celdas_impares = 4;   //  núm. de entradas del buffer v1
 int                        // variables permanentes
   v2[num_celdas_pares],//  buffer de tamaño fijo donde introduciran las hebras pares
   v1[num_celdas_impares], //buffer de tamaño fijo donde introducirán las hebras impares
   primera_libre_pares,            //indica la celda de la proxima inserción
   primera_libre_impares;          //  indice de celda de la próxima inserción
 CondVar         // colas condicion:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres_pares,                    //cola donde espera el productor par
   libres_impares ;                 //  cola donde espera el productor impar (n<num_celdas_total)

 public:                    // constructor y métodos públicos
   ProdCons2SU(  ) ;           // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir_pares( int valor ); // insertar un valor (sentencia E) (productor)
   void escribir_impares( int valor );
} ;
// -----------------------------------------------------------------------------

ProdCons2SU::ProdCons2SU()
{
   primera_libre_pares = 0 ;
   primera_libre_impares = 0;
   libres_pares=newCondVar();
   ocupadas=newCondVar();
   libres_impares=newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdCons2SU::leer(  )
{

   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   if ( primera_libre_pares == 0 && primera_libre_impares==0)
      ocupadas.wait();

   int valor=0;

   if(primera_libre_pares>0 && primera_libre_impares>0) {
   
      int de_donde_leer = aleatorio<1,2>();

      if (de_donde_leer = 2){
          primera_libre_pares-- ;
          valor = v2[primera_libre_pares] ;
          libres_pares.signal();
      }
      else{
          primera_libre_impares--;
          valor = v1[primera_libre_impares];
          libres_impares.signal();
      }
   }
   else if(primera_libre_pares>0){
      primera_libre_pares-- ;
      valor = v2[primera_libre_pares] ;
      libres_pares.signal();
   }
   else {
      primera_libre_impares--;
      valor = v1[primera_libre_impares];
      libres_impares.signal();
   }


   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void ProdCons2SU::escribir_pares( int valor )
{

   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   if ( primera_libre_pares == num_celdas_pares )
      libres_pares.wait();

   // hacer la operación de inserción, actualizando estado del monitor
   v2[primera_libre_pares] = valor ;
   primera_libre_pares++ ;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}
// *****************************************************************************

void ProdCons2SU::escribir_impares( int valor )
{

   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   if ( primera_libre_impares == num_celdas_impares )
      libres_impares.wait();

   // hacer la operación de inserción, actualizando estado del monitor
   v1[primera_libre_impares] = valor ;
   primera_libre_impares++ ;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}

// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<ProdCons2SU> monitor, int numero_hebra )
{
   for( unsigned i = 0 ; i < producir_por_hebra ; i++ )
   {
      int valor = producir_dato(numero_hebra) ;

      if(numero_hebra%2==0)
         monitor->escribir_pares( valor );
      else
        monitor->escribir_impares(valor);
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdCons2SU> monitor, int numero_hebra )
{
   for( unsigned i = 0 ; i < consumir_por_hebra ; i++ )
   {
      int valor = monitor->leer();
      consumir_dato( numero_hebra, valor ) ;
   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (1 prod/cons, Monitor SC, buffer FIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<ProdCons2SU> monitor = Create<ProdCons2SU>();
   
   thread hebra_productora[numero_productores];
   thread hebra_consumidora[numero_consumidores];

   for (int i=0;i<numero_productores;i++)
       hebra_productora[i] = thread(funcion_hebra_productora, monitor, i);
   for (int i =0;i<numero_consumidores;i++)
       hebra_consumidora[i] = thread(funcion_hebra_consumidora, monitor, i);

   for (int i=0;i<numero_productores;i++)
       hebra_productora[i].join();
   for (int i =0;i<numero_consumidores;i++)
       hebra_consumidora[i].join();

   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}
