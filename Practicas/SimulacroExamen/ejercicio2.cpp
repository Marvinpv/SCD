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

class Peaje : public HoareMonitor
{
 private:
 static const int           // constantes:
   num_cabinas = 2;        //  núm. de cabinas
 int                        // variables permanentes
   coches_por_cabina[num_cabinas];

 CondVar        // colas condicion:
   cabinas[num_cabinas]   ;             //  cabinas del peaje

 public:                    // constructor y métodos públicos
   Peaje(  ) ;           // constructor
   int llegada_peaje();
   void pagado(int cab);
} ;

Peaje ::Peaje(){
    for(int i = 0 ; i < num_cabinas ; i++){
        coches_por_cabina[i] = 0;
        cabinas[i] = newCondVar();
    }
        



}

int Peaje::llegada_peaje(){
    int cab_con_menos_coches = 0, min = coches_por_cabina[0];
    
    for(int i = 0 ; i < num_cabinas ; i++){
        if(coches_por_cabina[i] < min){
            min = coches_por_cabina[i];
            cab_con_menos_coches = i;
        }
    }

    

    if(coches_por_cabina[cab_con_menos_coches] > 0){
        coches_por_cabina[cab_con_menos_coches]++;
        cabinas[cab_con_menos_coches].wait();
    }
        
    

    return cab_con_menos_coches;

}

void Peaje::pagado(int cab){
    if(coches_por_cabina[cab]>0){
        cabinas[cab].signal();
        coches_por_cabina[cab]--;
    }
    
}

void Hebra_coche(MRef<Peaje>  monitor ){
    while(true){
        int cabina = monitor->llegada_peaje();
        cout<<"Entra coche en la cabina "<<cabina<<endl;
        chrono::milliseconds duracion_cola1( aleatorio<10,100>() );
        this_thread::sleep_for( duracion_cola1 );

        monitor->pagado(cabina);

        cout<<"Coche de la cabina "<<cabina<<"Ha terminado su cola"<<endl;
        chrono::milliseconds duracion_cola2( aleatorio<10,100>() );
        this_thread::sleep_for( duracion_cola2 );

    }
}   

int main(){
    
    const int coches = 50;

    cout << "-------------------------------------------------------------------------------" << endl
        << "Problema del peaje de examen" << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<Peaje> monitor = Create<Peaje>();

   thread coche[coches];

   for(int i = 0 ; i < coches ; i++){
          coche[i] = thread(Hebra_coche, monitor);
    }

    for(int i = 0 ; i < coches ; i++){
              coche[i].join();
    }
}