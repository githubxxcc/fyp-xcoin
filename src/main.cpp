

#include <boost/thread.hpp>
//#include <boost/chrono.hpp>

#include <iostream>



void PoW() 
{
    std::cout << "THREAD:: PoW" << std::endl;
}


void GenTx () 
{
    std::cout << "Thread::GenTx" << std::endl;
}


void ValidateTx () 
{
    std::cout << "THREAD:: VALIDATE" << std::endl;
}

int main() 
{
    boost::thread t1{GenTx};
    boost::thread t2{ValidateTx};
    boost::thread t3{PoW};

    t1.join();
    t2.join();
    t3.join();
}




