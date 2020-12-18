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

const int num_lectores   = 5,       //Número de lectores del programa
          num_escritores = 2;


class LecEsc : public HoareMonitor
{
 private:
 

 CondVar                                      // colas condicion:
   lectura,                                //  cola donde espera el consumidor (n>0)
   escritura;                 //  cola donde espera el productor  (n<num_celdas_total)

 bool
   escrib;

 int    
   n_lec;

 public:                    // constructor y métodos públicos
   LecEsc(  ) ;                 // constructor
   void ini_lectura();
   void fin_lectura();          
   void ini_escritura();
   void fin_escritura();    
} ;
// -----------------------------------------------------------------------------

LecEsc::LecEsc(  )
{
    lectura = newCondVar();
    escritura = newCondVar();
    n_lec = 0;
    escrib = false;
}

void LecEsc::ini_lectura(){
    if(escrib)
        lectura.wait();
    
    n_lec++;

    lectura.signal();
}

void LecEsc::fin_lectura(){
    n_lec--;

    if(n_lec == 0)
        escritura.signal();
}

void LecEsc::ini_escritura(){
    if(n_lec > 0 || escrib)
        escritura.wait();
    
    escrib = true;
}

void LecEsc::fin_escritura(){
    escrib = false;

    if(!lectura.empty())
        lectura.signal();
    else
        escritura.signal();
}

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)



//----------------------------------------------------------------------
// función que ejecuta la hebra del lector

void proceso_lector( MRef<LecEsc> monitor, int id)
{
    while(true){
        monitor->ini_lectura();
        cout<<"Lector "<<id<<" empieza a leer datos"<<endl;
        chrono::milliseconds duracion_leer( aleatorio<10,100>() );
        this_thread::sleep_for( duracion_leer );
        cout<<"Lector "<<id<<" ha terminado de leer datos"<<endl;
        monitor->fin_lectura();
        this_thread::sleep_for( std::chrono::milliseconds (aleatorio<10,100>()) );
    }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra



//----------------------------------------------------------------------
// función que ejecuta la hebra del escritor
void  proceso_escritor(MRef<LecEsc> monitor, int id )
{
    while(true){
        monitor->ini_escritura();
        cout<<"Escritor "<<id<<" comienza a escribir"<<endl;
        chrono::milliseconds duracion_escribir( aleatorio<10,100>() );
        this_thread::sleep_for( duracion_escribir );
        cout<<"Escritor "<<id<<" ha terminado de escribir"<<endl;
        monitor->fin_escritura();
    }
}

//----------------------------------------------------------------------

int main()
{
   
   
   
   cout << "--------------------------------------------------------" << endl
        << "           Problema de los lectores-escritores          " << endl
        << "--------------------------------------------------------" << endl
        << flush ;
    

    MRef<LecEsc> monitor = Create<LecEsc>();

   thread lectores[num_lectores] ,escritores[num_escritores];
   for(int i = 0 ;i < num_lectores ; i++){
      lectores[i] = thread(proceso_lector,monitor,i);
   }

   for(int i = 0 ;i < num_escritores ; i++){
      escritores[i] = thread(proceso_escritor,monitor,i);
   }

   for(int i = 0 ;i < num_lectores ; i++){
      lectores[i].join();
   }

   for(int i = 0 ;i < num_escritores ; i++){
      escritores[i].join();
   }

   return 0;

}