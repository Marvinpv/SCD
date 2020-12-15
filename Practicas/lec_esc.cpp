#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "HoareMonitor.h"

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

class Lec_esc : public HoareMonitor
{
    private:
        int n_lec;
        bool escrib;
        CondVar lectura,escritura;
    
    public:
        Lec_esc();
        void ini_lectura();
        void ini_escritura();
        void fin_lectura();
        void fin_escritura();
};

Lec_esc::Lec_esc(){
    n_lec = 0;
    escrib = false;
    lectura = newCondVar();
    escritura = newCondVar();
}

void Lec_esc::ini_lectura(){
    if(escrib)
        lectura.wait();
    
    n_lec += 1;

    
    lectura.signal();
}

void Lec_esc::fin_lectura(){
    n_lec -= 1;

    if(n_lec == escritura.get_nwt())
        escritura.signal();
}

void Lec_esc::ini_escritura(){
    if(n_lec > 0 || escrib)
        escritura.wait();
    
    escrib = true;
}


void Lec_esc::fin_escritura(){
    escrib = false;

    if(lectura.get_nwt() == escritura.get_nwt())
        int aleatorio = aleatorio<0,1>()
        if(aleatorio = 0 )
            lectura.signal();
        else
            escritura.signal();
        
    else
        if(!lectura.empty() && n_lec < escritura.get_nwt())
            lectura.signal();
        else
            escritura.signal();
    
}


void funcion_lector(MRef<Lec_esc> monitor,int indice){
    while(true){
        cout<<"Lector numero "<<indice<<" comienza la espera..."<<endl;
        monitor->ini_lectura();
        cout<<"Lector numero "<<indice<<" comienza a leer..."<<endl;    
        monitor->fin_lectura();
        cout<<"Lector numero "<<indice<<" termina de leer..."<<endl;
    }
}

void funcion_escritor(MRef<Lec_esc> monitor,int indice){
    while(true){
        cout<<"Escritor numero "<<indice<<" comienza la espera..."<<endl;
        monitor->ini_lectura();
        cout<<"Escritor numero "<<indice<<" comienza a escribir..."<<endl;    
        monitor->fin_lectura();
        cout<<"Escritor numero "<<indice<<" termina de escribir..."<<endl;
    }
}

const int num_lectores = 5;
const int num_escritores = 5;

int main(){

    MRef<Lec_esc> monitor = Create<Lec_esc>();

    thread lectores[num_lectores];
    thread escritores[num_escritores];

    for(int i = 0 ;i < num_lectores ; i++){
      lectores[i] = thread(funcion_lector,monitor,i);
   }

   for(int i = 0 ;i < num_escritores ; i++){
      escritores[i] = thread(funcion_lector,monitor,i);
   }

    for(int i = 0 ;i < num_lectores ; i++){
      lectores[i].join();
   }

   for(int i = 0 ;i < num_escritores ; i++){
      escritores[i].join();
   }


}