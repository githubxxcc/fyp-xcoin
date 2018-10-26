#ifndef XCOIN_NET_H_
#define XCOIN_NET_H_

#include <sstream>
#include <string>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>


namespace xcoin {
    class MsgHeader;
    class PingMsg;

    class MsgHeader {
        public:
            int type;
            int size;
        
        MsgHeader(string & s) {
            stringstream ss(s);
            ss >> type >> size;
        }
        
        string to_string() {
            stringstream ss;
            ss << type << " " << size;
            return ss.str();
        }

        
    };

    class PingMsg {
        public:
            int64_t start_us;
            int hop;
            size_t data_size; // in KB
            string data;

        PingMsg() 
        {
            start_us = 0;
            hop = 0;
            data_size = 0;
        }

        PingMsg(int64_t start, int hop, size_t msg_size) 
            : start_us(start),
              hop(hop),
              data_size(msg_size << 10)
             {
                 data = string(data_size, 'x');
             }


        string to_string() {
            stringstream ss;
            ss  << "Start time: " << start_us   << "\t"
                << "Data:       " << data       << "\t" 
                << "Hop :       " << hop        << "\n";

            return ss.str();
        }

        string save() {
            stringstream ss;
            boost::archive::text_oarchive oa{ss};
            oa << *this;
            return ss.str();
        }
        static PingMsg deserialize(stringstream &ss) 
        {
            PingMsg msg;
            boost::archive::text_iarchive ia{ss};
            ia >> msg;
            return msg;
        }

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) 
        {
            ar  & start_us;
            ar  & hop;
            ar  & data_size;
            ar  & data;
        }
        
    };



}


#endif
