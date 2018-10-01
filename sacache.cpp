#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <cstring>
using namespace std;

class cacheLine
{
  public:
  cacheLine()
  {
    tag = -1;
    counter = 0;
    dirty = 0;
  }
  int tag; //converted from first two hex digits of addr
  string bytes [8];
  int dirty;
  int counter;
};

int bin_to_dec(int bin[], int length)
{
  int dec = 0;
  for(int i = length - 1; i >= 0; i--)
  {
    dec += (pow(2.0,i) * bin[length - 1 - i]);
  }
  return dec;
}

void hex_to_bin(string hex, int *binAddress)
{
  int decVal;
  int quotient;
  int rem;
  for(int i = 3; i >= 0; i--)
  {
    if((hex[i] - 'A') >= 0)
      decVal = hex[i] - 'A' + 10;
    else
      decVal = hex[i] - '0';
    quotient = decVal;
    for(int j = 3; j >= 0; j--)
    {
      rem = quotient % 2;
      quotient = quotient / 2;
      binAddress[i*4 + j] = rem;
      //cout << binAddress[i*4 + j];
    }  
  }
  //cout << endl;
}

int min_counter(cacheLine group [])
{
  int currMin = 0xFFFF;
  int minIndex = -1;
  for(int i = 0; i < 4; i++)
  {
    if((group[i]).counter < currMin)
    {
      currMin = group[i].counter;
      minIndex = i;
      //cout << minIndex << endl;
    }
    //cout << currMin << endl;
  }
  return minIndex;
} 

void write_to_RAM(cacheLine* writeBack, int set, string* RAM)
{
  int quotient = writeBack->tag;
  int destination [16];
  int RAMindex;
  int rem;
  for(int j = 7; j >= 0; j--)
  {
    rem = quotient % 2;
    quotient = quotient / 2;
    destination[j] = rem;
  }
  quotient = set;
  for(int j = 12; j >= 8; j--)
  {
    rem = quotient % 2;
    quotient = quotient / 2;
    destination[j] = rem;
  }
  for(int i = 0; i < 3; i++)
    destination[i+13] = 0;
  RAMindex = bin_to_dec(destination, 16);
  for(int i = 0; i < 8; i++)
  {
    RAM[RAMindex + i] = writeBack->bytes[i];
    //cout << RAM[RAMindex + i] << "," << writeBack->bytes[i] << "," << RAMindex + i << endl;
  }
}

void pull_from_RAM(cacheLine* writeTo, int binAddress[], string* RAM)
{
  int startAddress [16];
  for(int i = 0; i < 13; i++)
  {
    startAddress[i] = binAddress[i];
    //cout << startAddress[i];
  }
  //cout << endl;
  for(int i = 13; i < 16; i++)
    startAddress[i] = 0;
  int RAMindex = bin_to_dec(startAddress, 16);
  //cout << RAMindex << endl;
  for(int i = 0; i < 8; i++)
  {
    writeTo->bytes[i] = RAM[RAMindex + i];
    //cout << RAM[RAMindex + 11] << "," << RAM[RAMindex+i] << "," << writeTo->bytes[i] << endl;
  }
  writeTo->tag = bin_to_dec(startAddress, 8);
  //cout << startAddress[i] << "," << writeTo->tag[i] << "," << endl;
}

void read_and_output(ofstream& outfile, cacheLine readFrom, int offset, int hit)
{
  outfile << "Data,";
  outfile << readFrom.bytes[offset];
  outfile << ",Dirty,";
  outfile << readFrom.dirty;
  outfile << ",Hit,";
  outfile << hit;
  outfile << "\n";
}

int main(int argc, char const *argv[])
{
  int RAMSize = 65536;
  string RAM [RAMSize];
  for(int i = 0; i < RAMSize; i++)
    RAM[i] = "00";
  cacheLine cache [32][4]; //index of this array is equal to the set of the cacheLine
  ifstream addresses(argv[1]);
  ofstream outfile;
  outfile.open ("sa-out.csv");
  string address;
  string RW;
  string data;
  int binAddress [16];
  int taco; //nameholder for tag
  int tac; //tempholder for tag
  string tagStr;
  int set;
  int binSet [5];
  int offset;
  int binOffset [3];
  int globalCounter = 0;
  while(getline(addresses, address, ','))
  {
    globalCounter++;
    //cout << globalCounter << endl;
    taco = 0;
    getline(addresses, RW, ',');
    getline(addresses, data, '\n');
    hex_to_bin(address, binAddress);
    tagStr = address.substr(0,2);
    const char* ctag;
    ctag = tagStr.c_str();
    //cout << ctag[0] << ctag[1] << endl;
    for(int i = 0; i <= 1; i++)
    {
        if((ctag[i] - 'A') >= 0)
            tac = ctag[i] - 'A' + 10;
        else
            tac = ctag[i] - '0';
        taco += (tac * pow(16,1-i));
    }
    for(int i = 0; i < 5; i++)
    {
      binSet[i] = binAddress[i+8];
      //cout << binSet[i];
    }
    //cout << endl;
    set = bin_to_dec(binSet, 5);
    for(int i = 0; i < 3; i++)
    {
      binOffset[i] = binAddress[i+13];
      //cout << binOffset[i];
    }
    //cout << endl;
    offset = bin_to_dec(binOffset, 3);
    int thisOne = -1;
    for(int i = 0; i < 4; i++)
    {
      //cout << "taco: " << taco << ", tag: " << cache[set][i].tag << endl;
      if(cache[set][i].tag == taco)
      {
        thisOne = i;
        //cout << thisOne << endl;
        break;
      }
    }
    if(thisOne == -1) //miss
    {
      int evictThis = min_counter(cache[set]);
      //cout << evictThis << endl;
      if(cache[set][evictThis].dirty == 1) 
      {
        write_to_RAM(&(cache[set][evictThis]), set, RAM);
      }
      pull_from_RAM(&(cache[set][evictThis]), binAddress, RAM);
      cache[set][evictThis].counter = globalCounter;
      if(!RW.compare("00")) //if write
      {
        //cout << "write and miss" << endl;
        cache[set][evictThis].bytes[offset] = data;
        cache[set][evictThis].dirty = 1;
      }
      else if(!RW.compare("FF"))
      {
        //cout << "read and miss" << endl;
        read_and_output(outfile, cache[set][evictThis], offset, 0);
        cache[set][evictThis].dirty = 0;
      }
    }
    else if(thisOne >= 0 || thisOne <= 3)//hit
    {
      cache[set][thisOne].counter = globalCounter;
      if(!RW.compare("00")) //if write
      {
        //cout << "hit and write" << endl;
        cache[set][thisOne].bytes[offset] = data;
        cache[set][thisOne].dirty = 1;
      }
      else if(!RW.compare("FF"))
      {
        //cout << "hit and read" << endl;
        read_and_output(outfile, cache[set][thisOne], offset, 1);
      }
    }
  }
  outfile.close();
  return 0;
}