/*

  Project:       1Sheeld Library 
  File:          InternetShield.cpp
                 
  Version:       1.2

  Compiler:      Arduino avr-gcc 4.3.2

  Author:        Integreight
                 
  Date:          2015.1

*/
#include "OneSheeld.h"
#include "InternetShield.h"



InternetShield::InternetShield() : ShieldParent(INTERNET_ID)
{
	isSetOnErrorCallBackAssigned=false;
	for(int i=0;i<MAX_NO_OF_REQUESTS;i++){
		requestsArray[i]=NULL;
	}
}

bool InternetShield::addToRequestsArray(HttpRequest & request)
{
	int i;
	if(request.callbacksRequested!=0)
	{
		for (i = 0; i < MAX_NO_OF_REQUESTS; i++)
		{
			if(requestsArray[i]==&request)return true;
		}
		
		for (i = 0; i < MAX_NO_OF_REQUESTS; i++)
		{
			if(requestsArray[i]==NULL)break;
		}

		if(i>=MAX_NO_OF_REQUESTS)return false;
		else
		{
			requestsArray[i]=&request;
			return true;
		}
	}
	else {
		for (i = 0; i < MAX_NO_OF_REQUESTS; i++)
		{
			if(requestsArray[i]==&request)requestsArray[i]=NULL;
		}
		return true;
	}
}

bool InternetShield::performGet(HttpRequest & request)
{	
	bool isAdded=addToRequestsArray(request);
	if(isAdded)
	{
	
		OneSheeld.sendPacket(INTERNET_ID,0,INTERNET_GET,2,
							 new FunctionArg(sizeof(int),request.localRequestId),
							 new FunctionArg(1,&(request.callbacksRequested)));
	}
	
	return isAdded;
}

bool InternetShield::performPost(HttpRequest & request)
{	
	
	bool isAdded=addToRequestsArray(request);
	if(isAdded)
	{
		OneSheeld.sendPacket(INTERNET_ID,0,INTERNET_POST,2,
						 new FunctionArg(sizeof(int),request.localRequestId),
						 new FunctionArg(1,&(request.callbacksRequested)));
	}
	
	return isAdded;
}

bool InternetShield::performPut(HttpRequest & request)
{	
	bool isAdded=addToRequestsArray(request);
	if(isAdded)
	{
	
		OneSheeld.sendPacket(INTERNET_ID,0,INTERNET_PUT,2,
							 new FunctionArg(sizeof(int),request.localRequestId),
							 new FunctionArg(1,&(request.callbacksRequested)));
	}
	
	return isAdded;
}

bool InternetShield::performDelete(HttpRequest & request)
{	
	bool isAdded=addToRequestsArray(request);
	if(isAdded)
	{
	
		OneSheeld.sendPacket(INTERNET_ID,0,INTERNET_DELETE,2,
							 new FunctionArg(sizeof(int),request.localRequestId),
							 new FunctionArg(1,&(request.callbacksRequested)));
	}
	
	return isAdded;
}

void InternetShield::ignoreResponse(HttpRequest & request)
{
	request.ignoreResponse();
}

void InternetShield::cancelAllRequests(void)
{
	OneSheeld.sendPacket(INTERNET_ID,0,INTERNET_CANCEL_ALL_REQUESTS,0);
}

void InternetShield::setBasicAuthentication(char * userName,char * password)
{
	OneSheeld.sendPacket(INTERNET_ID,0,INTERNET_SET_AUTHENTICATION,2,
		                 new FunctionArg(strlen(userName),(byte*)userName),
		                 new FunctionArg(strlen(password),(byte*)password));
}

void InternetShield::clearBasicAuthentication()
{
	OneSheeld.sendPacket(INTERNET_ID,0,INTERNET_CLEAR_AUTHENTICATIOM,0);
}

void InternetShield::setDefaultMaxResponseBytesCount(int size)
{
	byte sizeArray[2] ;
  	sizeArray[1] = (size >> 8) & 0xFF;
  	sizeArray[0] = size & 0xFF;
	OneSheeld.sendPacket(INTERNET_ID,0,INTERNET_SET_DEFAULT_MAX_RESPONSE,1,new FunctionArg(sizeof(int),sizeArray));
}

void InternetShield::processData()
{
	byte functionId = OneSheeld.getFunctionId();

	if(functionId == HTTP_GET_SUCCESS   || functionId == HTTP_GET_FAILURE           ||
	   functionId == HTTP_GET_STARTED   || functionId == HTTP_GET_ON_PROGRESS       ||
	   functionId == HTTP_GET_ON_FINISH || functionId == RESPONSE_GET_NEXT_RESPONSE ||
	   functionId == RESPONSE_GET_ERROR || functionId == RESPONSE_INPUT_GET_HEADER	||
	   functionId == RESPONSE_GET_JSON  || functionId == RESPONSE_GET_JSON_ARRAY_LENGTH)
	{

		int requestId  = OneSheeld.getArgumentData(0)[0]|((OneSheeld.getArgumentData(0)[1])<<8);
		unsigned long totalBytesCount  = 0;
		unsigned long progressDoneBytes  = 0;
		int statusCode = 0;
		int error = 0;

		if(functionId ==HTTP_GET_SUCCESS||functionId == HTTP_GET_FAILURE)
			statusCode = OneSheeld.getArgumentData(1)[0]|((OneSheeld.getArgumentData(1)[1])<<8);
		else if (functionId == RESPONSE_GET_ERROR)
			error=OneSheeld.getArgumentData(1)[0];
		
		else if(functionId == HTTP_GET_ON_PROGRESS)
		{	
			progressDoneBytes  =(unsigned long)OneSheeld.getArgumentData(1)[0]
						 		|(((unsigned long)OneSheeld.getArgumentData(1)[1])<<8)
						 		|(((unsigned long)OneSheeld.getArgumentData(1)[2])<<16)
						 		|(((unsigned long)OneSheeld.getArgumentData(1)[3])<<24); 
		}


		if(functionId == HTTP_GET_SUCCESS||functionId == HTTP_GET_FAILURE||functionId == HTTP_GET_ON_PROGRESS)
		{	
			 totalBytesCount  =(unsigned long)OneSheeld.getArgumentData(2)[0]
						 		|(((unsigned long)OneSheeld.getArgumentData(2)[1])<<8)
						 		|(((unsigned long)OneSheeld.getArgumentData(2)[2])<<16)
						 		|(((unsigned long)OneSheeld.getArgumentData(2)[3])<<24); 
		}
		
		char * data =NULL;
		int dataLength=0;
		int i;
		for (i = 0; i < MAX_NO_OF_REQUESTS; i++)
		{  
			if(requestsArray[i]!=NULL&&requestsArray[i]->getId()==requestId)
			{
				if((((requestsArray[i]->callbacksRequested) & SUCCESS_CALLBACK_BIT) && functionId == HTTP_GET_SUCCESS)||
				   (((requestsArray[i]->callbacksRequested) & FAILURE_CALLBACK_BIT) && functionId == HTTP_GET_FAILURE)||
				   (((requestsArray[i]->response.callbacksRequested) & RESPONSE_GET_NEXT_RESPONSE_BIT) && functionId == RESPONSE_GET_NEXT_RESPONSE))
				{
					byte dataArgumentNumber; 

					if(functionId == RESPONSE_GET_NEXT_RESPONSE)
					{
						dataArgumentNumber = 1;
						if(requestsArray[i]->response.bytesCount!= 0 && requestsArray[i]->response.isInit && requestsArray[i]->response.bytes!=NULL)
						{
							free(requestsArray[i]->response.bytes);
							requestsArray[i]->response.bytes = NULL;
						}
					}
					else
					{
						dataArgumentNumber = 3;
						requestsArray[i]->response.dispose(false);
					}
					
					if((functionId == RESPONSE_GET_NEXT_RESPONSE && requestsArray[i]->response.isInit) ||
					  ((functionId == HTTP_GET_SUCCESS||functionId == HTTP_GET_FAILURE) && totalBytesCount!=0))
					{
						dataLength = OneSheeld.getArgumentLength(dataArgumentNumber);
						if(dataLength!=0)
						{
							data=(char*)malloc(sizeof(char)*(dataLength+1));
							for (int j=0; j<dataLength; j++)
							{
								data[j]=OneSheeld.getArgumentData(dataArgumentNumber)[j];
							}

						data[dataLength]='\0';
						requestsArray[i]->response.bytesCount=dataLength;
						requestsArray[i]->response.index+=dataLength;
						requestsArray[i]->response.bytes=data;
						}
						else
						{
							requestsArray[i]->response.bytesCount=0;
							requestsArray[i]->response.bytes=NULL;
						}
					}

					if(functionId != RESPONSE_GET_NEXT_RESPONSE)
					{
						requestsArray[i]->response.isInit=true;
						requestsArray[i]->response.isDisposedTriggered=false;
						requestsArray[i]->response.statusCode=statusCode;
						requestsArray[i]->response.totalBytesCount=totalBytesCount;
					}
					if(((requestsArray[i]->callbacksRequested) & SUCCESS_CALLBACK_BIT) && functionId == HTTP_GET_SUCCESS)
						{
							if(!OneSheeld.isInACallback())
							{
								OneSheeld.enteringACallback();
								requestsArray[i]->successCallBack(requestsArray[i]->response);
								OneSheeld.exitingACallback();
							}
						}
					else if(((requestsArray[i]->callbacksRequested) & FAILURE_CALLBACK_BIT) && functionId == HTTP_GET_FAILURE)
						{
							if(!OneSheeld.isInACallback())
							{
								OneSheeld.enteringACallback();
								requestsArray[i]->failureCallBack(requestsArray[i]->response);
								OneSheeld.exitingACallback();
							}
						}
					else if((requestsArray[i]->response.callbacksRequested) & RESPONSE_GET_NEXT_RESPONSE_BIT)
						{
							if(!OneSheeld.isInACallback())
							{
								OneSheeld.enteringACallback();
								requestsArray[i]->response.getNextCallBack(requestsArray[i]->response);
								OneSheeld.exitingACallback();
							}
						}
				}
				else if(((requestsArray[i]->callbacksRequested) & START_CALLBACK_BIT) && functionId == HTTP_GET_STARTED)
				{
					if(!OneSheeld.isInACallback())
					{
						OneSheeld.enteringACallback();
						requestsArray[i]->startCallBack();
						OneSheeld.exitingACallback();
					}
				}
				else if(((requestsArray[i]->callbacksRequested) & FINISH_CALLBACK_BIT) && functionId == HTTP_GET_ON_FINISH)
				{
					if(!OneSheeld.isInACallback())
					{
						OneSheeld.enteringACallback();
						requestsArray[i]->finishCallBack();
						OneSheeld.exitingACallback();
					}
				}
				else if(((requestsArray[i]->callbacksRequested) & PROGRESS_CALLBACK_BIT) && functionId == HTTP_GET_ON_PROGRESS)
				{
					if(!OneSheeld.isInACallback())
					{
						OneSheeld.enteringACallback();
						requestsArray[i]->progressCallback(progressDoneBytes,totalBytesCount);
						OneSheeld.exitingACallback();
					}
				}
				else if(((requestsArray[i]->response.callbacksRequested) & RESPONSE_GET_ERROR_BIT) && functionId == RESPONSE_GET_ERROR) 
				{
					if(!OneSheeld.isInACallback())
					{
						OneSheeld.enteringACallback();
						requestsArray[i]->response.getErrorCallBack(error);	
						OneSheeld.exitingACallback();
					}
				}
				else if(((requestsArray[i]->response.callbacksRequested) & RESPONSE_INPUT_GET_HEADER_BIT) && functionId == RESPONSE_INPUT_GET_HEADER)
				{
					byte headerNameLength = OneSheeld.getArgumentLength(1);

					char  headerName [headerNameLength+1];
					for(int k = 0 ;k<headerNameLength ;k++)
					{
						headerName[k]=OneSheeld.getArgumentData(1)[k];
					}
					headerName[headerNameLength]='\0';


					byte headerValueLength = OneSheeld.getArgumentLength(2);

					char  headerValue [headerValueLength+1];
					for(int j = 0 ;j<headerValueLength ;j++)
					{
						headerValue[j]=OneSheeld.getArgumentData(2)[j];
					}
					headerValue[headerValueLength]='\0';
					if(!OneSheeld.isInACallback())
					{
						OneSheeld.enteringACallback();
						requestsArray[i]->response.getHeaderCallBack(headerName,headerValue);
						OneSheeld.exitingACallback();
					}
				}
				else if(functionId == RESPONSE_GET_JSON || functionId == RESPONSE_GET_JSON_ARRAY_LENGTH)
				{
					int keyChainTypes = OneSheeld.getArgumentData(2)[0]|((OneSheeld.getArgumentData(2)[1])<<8);

					byte argumentNo = OneSheeld.getArgumentNo();
					if(argumentNo - 3 <= MAX_JSON_KEY_DEPTH)
					{
						JsonKeyChain responseJsonChain;
						for(int j=3;j<argumentNo;j++)
						{
							if((keyChainTypes & (1 << (j-3))))
							{
								byte jsonKeyValueLength = OneSheeld.getArgumentLength(j);
								char  jsonKeyValue[jsonKeyValueLength+1];
								for(int k = 0 ;k<jsonKeyValueLength ;k++)
								{
									jsonKeyValue[k]=OneSheeld.getArgumentData(j)[k];
								}
								jsonKeyValue[jsonKeyValueLength]='\0';
								responseJsonChain[jsonKeyValue];
							}
							else
							{
								int jsonKeyValue = OneSheeld.getArgumentData(j)[0]|((OneSheeld.getArgumentData(j)[1])<<8);
								responseJsonChain[jsonKeyValue];
							}
						}

						if(((requestsArray[i]->response.callbacksRequested) & RESPONSE_GET_JSON_BIT) && functionId == RESPONSE_GET_JSON)
						{
							byte jsonResponseLength = OneSheeld.getArgumentLength(1);

							char  jsonResponseValue[jsonResponseLength+1];
							for(int k = 0 ;k<jsonResponseLength ;k++)
							{
								jsonResponseValue[k]=OneSheeld.getArgumentData(1)[k];
							}
							jsonResponseValue[jsonResponseLength]='\0';
							if(!OneSheeld.isInACallback())
							{
								OneSheeld.enteringACallback(); 	
								requestsArray[i]->response.getJsonCallBack(responseJsonChain,jsonResponseValue);
								OneSheeld.exitingACallback();
							}
						}
						else if(((requestsArray[i]->response.callbacksRequested) & RESPONSE_GET_JSON_ARRAY_LENGTH_BIT) && functionId == RESPONSE_GET_JSON_ARRAY_LENGTH)
						{
							unsigned long arrayLength  =(unsigned long)OneSheeld.getArgumentData(1)[0]
							 		|(((unsigned long)OneSheeld.getArgumentData(1)[1])<<8)
							 		|(((unsigned long)OneSheeld.getArgumentData(1)[2])<<16)
							 		|(((unsigned long)OneSheeld.getArgumentData(1)[3])<<24);
							if(!OneSheeld.isInACallback())
							{
								OneSheeld.enteringACallback(); 		
								requestsArray[i]->response.getJsonArrayLengthCallBack(responseJsonChain,arrayLength);
								OneSheeld.exitingACallback();
							}
						}
					}
				}
				break;
			}
		}	

	}
	else if(functionId == INTERNET_GET_ERROR && isSetOnErrorCallBackAssigned)
	{
		int reqid  = OneSheeld.getArgumentData(0)[0]|((OneSheeld.getArgumentData(0)[1])<<8);
		int errorNumber  = OneSheeld.getArgumentData(1)[0];
		if(!OneSheeld.isInACallback())
		{
			OneSheeld.enteringACallback();
			(*internetErrorCallBack)(reqid,errorNumber);
			OneSheeld.exitingACallback();
		}
	}
}

void InternetShield::setOnError(void (*userFunction)(int requestid, int errorNumber))
{
	isSetOnErrorCallBackAssigned = true;
	internetErrorCallBack = userFunction;
}

#ifdef INTERNET_SHIELD
//Instatntiating Object
InternetShield Internet;
#endif