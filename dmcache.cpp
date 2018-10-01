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
    for(int i = 0; i < 7; i++)
      tag[i] = -1;
    dirty = 0;
  }
  int tag [7];
  string bytes [16];
  int dirty;
  //set is equal to the array of cacheLine's index
};

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

int bin_to_dec(int bin[], int length)
{
  int dec = 0;
  for(int i = length - 1; i >= 0; i--)
  {
    dec += (pow(2.0,i) * bin[length - 1 - i]);
  }
  return dec;
}

void write_to_RAM(cacheLine* writeBack, int set, string* RAM)
{
  int quotient = set;
  int destination [16];
  int RAMindex;
  int rem;
  for(int i = 0; i < 7; i++)
    destination[i] = writeBack->tag[i];
  for(int i = 4; i >= 0; i--)
  {
    rem = quotient % 2;
    quotient /= 2;
    destination[i+7] = rem;
  }
  for(int i = 0; i < 4; i++)
    destination[i+12] = 0;
  RAMindex = bin_to_dec(destination, 16);
  for(int i = 0; i < 16; i++)
  {
    RAM[RAMindex + i] = writeBack->bytes[i];
    //cout << RAM[RAMindex + i] << "," << writeBack->bytes[i] << "," << RAMindex + i << endl;
  }
}

void pull_from_RAM(cacheLine* writeTo, int binAddress[], string* RAM)
{
  int startAddress [16];
  for(int i = 0; i < 12; i++)
  {
    startAddress[i] = binAddress[i];
    //cout << startAddress[i];
  }
  //cout << endl;
  for(int i = 12; i < 16; i++)
    startAddress[i] = 0;
  int RAMindex = bin_to_dec(startAddress, 16);
  //cout << RAMindex << endl;
  for(int i = 0; i < 16; i++)
  {
    writeTo->bytes[i] = RAM[RAMindex + i];
    //cout << RAM[RAMindex + 11] << "," << RAM[RAMindex+i] << "," << writeTo->bytes[i] << endl;
  }
  for(int i = 0; i < 7; i++)
  {
    writeTo->tag[i] = startAddress[i];
    //cout << startAddress[i] << "," << writeTo->tag[i] << "," << endl;
  }
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

void print_check(string address, string RW, string data, int* binAddress, int set, int offset, bool equal)
{
  cout << "address: " << address << ", RW: " << RW << ", data: " << data << ", set: " << set << ", offset: " << offset << ", equal : " << equal << endl;
}

int main(int argc, char const *argv[])
{
  int RAMSize = 65536;
  string RAM [RAMSize];
  for(int i = 0; i < RAMSize; i++)
    RAM[i] = "00";
  cacheLine cache [32]; //index of this array is equal to the set of the cacheLine
  ifstream addresses(argv[1]);
  ofstream outfile;
  outfile.open ("dm-out.csv");
  string address;
  string RW;
  string data;
  int binAddress [16];
  int taco [7]; //nameholder for tag
  int set;
  int binSet [5];
  int offset;
  string offsetStr;
  char cStr;
  while(getline(addresses, address, ','))
  {
    getline(addresses, RW, ',');
    getline(addresses, data, '\n');
    hex_to_bin(address, binAddress);
    for(int i = 0; i < 7; i++)
      taco[i] = binAddress[i];
    for(int i = 0; i < 5; i++)
    {
      binSet[i] = binAddress[i+7];
      //cout << binSet[i];
    }
    //cout << endl;
    set = bin_to_dec(binSet, 5);
    offsetStr = address.substr(3, 1);
    cStr = offsetStr.at(0);
    if((cStr - 'A') >= 0)
      offset = cStr - 'A' + 10;
    else
      offset = cStr - '0';
    //cout << offset << endl;
    bool equal = true;
    for(int i = 0; i < 7; i++)
    {
      if(cache[set].tag[i] != taco[i])
        equal = false;
    }
    //print_check(address, RW, data, binAddress, set, offset, equal);
    if(!equal) //misses
    {
      if(!RW.compare("00")) //if write
      {
        if(cache[set].dirty == 1) //if current dirty is 1
          write_to_RAM(&(cache[set]), set, RAM);
        pull_from_RAM(&(cache[set]), binAddress, RAM);
        cache[set].bytes[offset] = data;
        //cout << set << endl;
        //cout << offset << endl;
        cache[set].dirty = 1;
        //cout << "write and miss" << endl;
      }
      else if(!RW.compare("FF"))
      {
        if(cache[set].dirty == 1) //if current dirty is 1
          write_to_RAM(&(cache[set]), set, RAM);
        pull_from_RAM(&(cache[set]), binAddress, RAM);
        read_and_output(outfile, cache[set], offset, 0);
        cache[set].dirty = 0;
        //cout << "read and miss" << endl;
      }
    }
    else //hits
    {
      if(!RW.compare("00"))
      {
        cache[set].bytes[offset] = data;
        cache[set].dirty = 1;
        //cout << "write and hit" << endl;
      }
      else if(!RW.compare("FF"))
      {
        read_and_output(outfile, cache[set], offset, 1);
        //cout << "read and hit" << endl;
      }
    }
  }
  outfile.close();
  return 0;
}