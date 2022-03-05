#ifndef Battery_h
#define Battery_h

class Battery
{
    public :
        Battery();
        void init();
        void measure();
        float voltage;
        int load;
};

#endif