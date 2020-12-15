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

const int num_coches = 8;
int coches_en_1=0,
    coches_en_2=0;

class Peaje : public HoareMonitor{

private:

CondVar c_cabina1, c_cabina2 ;

public:
   Peaje();
   int llegada_peaje();
   void pagado(int cab);
};

Peaje::Peaje(){
   c_cabina1 = newCondVar();
   c_cabina2 = newCondVar();
}

int Peaje::llegada_peaje(){

   int x;

   if(coches_en_1<=coches_en_2){
      x = 1;
      coches_en_1++;
      cout << "Coche nuevo en cabina 1 " << endl;
      if(coches_en_1>1){
         cout << "Coche espera a que termine de pagar en cola 1 " << endl;
         c_cabina1.wait();
      }
   }
   else{
      x = 2;
      coches_en_2++;
      cout << "Coche nuevo en cabina 2 " << endl;
      if(coches_en_2>1){
         cout << "Coche espera a que termine de pagar en cola 2 " << endl;
         c_cabina2.wait();
      }
   }

   return x;
}

void Peaje::pagado(int cab){

   if(cab==1){
      coches_en_1--;
      cout << "\tCoche en cabina 1 pagado, siguiente coche entra " << endl;
      c_cabina1.signal();
   }
   if(cab==2){
      coches_en_2--;
      cout << "\tCoche en cabina 2 pagado, siguiente coche entra " << endl;
      c_cabina2.signal();
   }
}

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

void espera_pago_peaje(){
  chrono::milliseconds duracion_espera( aleatorio<500,2000>() );
  this_thread::sleep_for(duracion_espera);
}

void espera_llegada_peaje(){
  chrono::milliseconds duracion_espera( aleatorio<500,1000>() );
  this_thread::sleep_for(duracion_espera);
}


void funcion_hebra_coche(MRef<Peaje> monitor, int i){
   int cabina;
   while(true){

      espera_llegada_peaje();
      cout << "Coche " << i << " ha llegado..." << endl;
      
      cabina = monitor->llegada_peaje();
      
      espera_pago_peaje();
      
      monitor->pagado(cabina);
      
      cout << "Coche " << i << " acaba de pagar y se retira... " << endl;
      

   }

}

int main(int argc, char const *argv[]) {
  MRef<Peaje> monitor = Create<Peaje>();

  thread hebras_coches[num_coches];
  

  for(int i = 0; i < num_coches; i++)
    hebras_coches[i] = thread ( funcion_hebra_coche, monitor,i);


  for(int i = 0; i < num_coches; i++)
    hebras_coches[i].join();
}


