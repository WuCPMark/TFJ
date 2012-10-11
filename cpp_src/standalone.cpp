#include <stdexcept>

#include <iostream>
#include <cstdio>
#include <cassert>
#include <unistd.h>

/* headers from our project */
#include "codegen.h"
#include "importxml.h"
#include "helper.h"

using namespace std;
using namespace xercesc;
using namespace llvm;

int main (int argc, char* argv[]) 
{
  char *xmlFile = NULL;
  int c;
  xmlProgram *p;
  bool en_vec = true;
  bool en_omp = true;
  bool en_unroll = false;
  bool en_pe = false;
  bool en_shift = true;
  int vec_len = 4;
  int block_amt = 256;
  bool en_fusion = false;
  bool en_aligned = false;
  while((c = getopt(argc, argv, "x:v:p:u:l:b:a:s:f:"))!=-1)
    {
      switch(c)
	{
	case 'b':
	  en_pe = (atoi(optarg)>0) ? true : false;
	  break;
	case 'a':
	  block_amt = (int)atoi(optarg);
	  break;
	case 'f':
	  en_fusion = (atoi(optarg)>0) ? true : false;
	  break;
	case 'x':
	  xmlFile = optarg;
	  break;
	case 'v':
	  en_vec = (atoi(optarg) > 0) ? true : false;
	  break;
	case 'p':
	  en_omp = (atoi(optarg) > 0) ? true : false;
	  break;
	case 's':
	  en_shift = (atoi(optarg) > 0 ) ? true : false;
	  break;
	case 'u':
	  en_unroll = (atoi(optarg) > 0) ? true : false;
	  break;
	case 'l':
	  vec_len = (int)atoi(optarg);
	  break;
	}
    }

  if(!isPow2(vec_len))
    {
      printf("%d is anon-power of 2 vector length, disabling vectorization\n",
	     vec_len);
      en_vec = false;
    }


  if(!xmlFile)
    {
      printf("no input file\n");
      return 0;
    }
  

  printf("Vectorization = %s\n", en_vec ? "enabled" : "disabled");
  printf("OpenMP annotation = %s\n", en_omp ? "enabled" : "disabled");
  printf("Inner loop unrolling = %s\n", en_unroll ? "enabled" : "disabled");
  printf("PE generation is %s \n", 
	 en_pe ? "enabled" : "disabled"); 

  try {
    XMLPlatformUtils::Initialize();
  }
  catch (const XMLException& toCatch) {
            char* message = XMLString::transcode(toCatch.getMessage());
            cout << "Error during initialization! :\n"
                 << message << "\n";
            XMLString::release(&message);
            return 1;
  }
  
  XercesDOMParser* parser = new XercesDOMParser();
  parser->setValidationScheme(XercesDOMParser::Val_Always);
  parser->setDoNamespaces(true);    // optional
  
  ErrorHandler* errHandler = (ErrorHandler*) new HandlerBase();
  parser->setErrorHandler(errHandler);
      
  try 
    {
      parser->parse(xmlFile);
      DOMDocument *xmlDoc = parser->getDocument();

      DOMElement *elementRoot = xmlDoc->getDocumentElement();
      if(!elementRoot) {throw(std::runtime_error("empty XML doc"));}
      std::map<std::string, ForStmt*> loopHash;

      p = new xmlProgram(en_vec, en_omp, en_unroll, 
			 en_pe, en_shift, block_amt, vec_len, en_fusion,
			 en_aligned);
      p->root = p;
      p->parseIt(elementRoot, loopHash);
      p->forStmtHash = loopHash;
      p->printProgram();
      bool perfectNest = false;
      p->runCodeGen(NULL,&perfectNest);
      p->dumpDot(std::string(xmlFile));
      //visitDOMTree(elementRoot, c);

      /*
      DOMNodeList*      children = elementRoot->getChildNodes();
      const  XMLSize_t nodeCount = children->getLength();
      for( XMLSize_t xx = 0; xx < nodeCount; ++xx )
	{
	  DOMNode* currentNode = children->item(xx);
	  visitDOMTree(currentNode, c);
	}
      */

    }
  catch (const XMLException& toCatch) 
    {
      char* message = XMLString::transcode(toCatch.getMessage());
      cout << "Exception message is: \n"
	   << message << "\n";
      XMLString::release(&message);
      return -1;
    }
  catch (const DOMException& toCatch) 
    {
      char* message = XMLString::transcode(toCatch.msg);
      cout << "Exception message is: \n"
	   << message << "\n";
      XMLString::release(&message);
      return -1;
    }
  catch (...) {
    cout << "Unexpected Exception \n" ;
    return -1;
  }
  
  if(p)
    {
      delete p;
    }


  delete parser;
  delete errHandler;
  return 0;
}
        
