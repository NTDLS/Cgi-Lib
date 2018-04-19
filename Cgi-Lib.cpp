#ifndef _CGI_LIB_CPP
#define _CGI_LIB_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_WARNINGS

/*********************************************************************************************
* This file contains the body for all the functions that will allow you to manipulate
* data that is sent over a web browser.  In order to use this file you must include
* the cgi-lib.h file in your source code.
*
* In documenting, when I refer to true I refer to the values of SUCCESS and when I
* refer to false, I refer to the value of FAILRE.
********************************************************************************************/

#include <Windows.H>
#include <StdIO.H>

#include "cgi-priv.h"
#include "cgi-lib.h"

static LIST *parse_data_record(const char *data, LIST *head);
static CGI_LIST parse_data_keyvalue(const char *record);
static char *unescape_url(const char *string);
static int num_encoded_data_items(const char *encodedStr);
static void Display_Entry(void *Data);
static int Compare(void *Data1, void *Data2);
static int Match(void *Data1, void *Data2);

/*----------------------------------------------------------------------------------------*/

/*
is_form_empty() -	This sub will return true if the form is empty otherwise it will
return false.  This sub can be used to test to see if the form is
empty so we don't waste time by calling the CGI_Input_Parse()
function.
*/
int is_form_empty(void)
{
	if(REQUEST_METHOD != NULL)
	{
		if (((strcmp(REQUEST_METHOD,GET))==0) && (strcmp(QUERY_STRING,"")==0))
		{
			return SUCCESS; /* the form is empty, not data via the QUERY STRING */
		}
		else if (((strcmp(REQUEST_METHOD,POST))==0) && CONTENT_LENGTH == 0)
		{
			return SUCCESS; /* the form is empty, no content via the post method */
		}
		else
			return FAILURE; /* The form is NOT empty */
	}
	else
		{
		//fprintf(stderr,"The REQUEST_METHOD environment variable\n");
		//fprintf(stderr,"does not exists.\n\n");
		//fprintf(stderr,"If you are dubugging, please set your CGI\n");
		//fprintf(stderr,"environment variables.\n\n");
		return FAILURE;
	} /* end of else statement for REQUEST_METHOD == NULL */

	return SUCCESS; /* the form is empty, no content via the post method */
}

/*----------------------------------------------------------------------------------------*/

/* cgi_err() -	This function will compose an error message for the application to use.  It
sends the proper mime types to the server when used and it terminates the
program.  The *msg1 variable is the name that will go between the H1 tags
and the *msg2 variable is the one that will go between the H2 tags.  This
function will also terminate the program and send a message to stderr.
*/
void cgi_err(char *msg1, char *msg2)
{
	printf("Content-type: text/html\n\n");

	puts("<HTML>");
	puts("<HEAD>");
	puts("<TITLE>Error!</TITLE>");
	puts("</HEAD>");
	puts("<BODY>");
	printf("<H1>%s</H1>\n",msg1);
	printf("<H2>%s</H2>\n",msg2);
	puts("</BODY>");
	puts("</HTML>");


	fprintf(stderr,"%s\n",msg2);

	exit(0);
}

/*----------------------------------------------------------------------------------------*/

/* CGI_Input_Parse() -	This function will take the raw data passed from the browser to the
server and adds each associated name and value to the linked list
entries.  It parses the information transmitted using both the
associated key and value to the linked list entries.  It parses
both teh GET and POST method.

This function "should be initialized at the beginning of the calling program", in order
to get the data into the linked list and be able to use it.  The value returned by the
function will be a pointer to the beginning of the list or a NULL if the list was not
created for whatever reason.	The user needs only to create a pointer variable of type
LIST.
*/
LIST *CGI_Input_Parse(void)
{
	char *postData=NULL; /* This variable will hold the post method data */
	unsigned int content_length=0;
	LIST *head=NULL;

	/* initialize our linked list */
	head = CreateList(0);


	if (REQUEST_METHOD != NULL)
	{
		if (strcmp(REQUEST_METHOD,GET) == 0) /* Then they match, it is the GET method */
		{
			/* if no query string data then return null */
			if (QUERY_STRING == NULL || strcmp(QUERY_STRING,"")==0)
			{
				free(head);
				return NULL;
			}
			head = parse_data_record(QUERY_STRING,head);
			return head; /* return a pointer to head */
		}
		else if (strcmp(REQUEST_METHOD,POST) == 0) /* Then they match, it is the post method */
		{
			if (CONTENT_LENGTH != NULL) /* make sure that we did receive data */
			{
				content_length = atoi(CONTENT_LENGTH); /* convert the string length to integer */
				if ((postData = (char *) calloc(sizeof(char), content_length + 1)) != NULL) /* allocate memory	*/
				{	/* check that everything was read into postData*/
					if((fread(postData,sizeof(char),content_length,stdin)) == content_length)
					{
						/* we need to terminate our data with a null char */
						postData[content_length]='\0';
						/* if all was read then parse the data */
						head = parse_data_record(postData,head);
						/* free the postData space since we have already used it for its intended purpose */
						free(postData);
						/* return a pointer to head */
						return head;
					}
					else /* if not everything was read then we must give an error message then stop */
					{
						cgi_err("Error!","Error Reading Data");
					}
				}
				else /* if not enough memory to allocate enough memory for postdata, tell the user and stop */
				{
					cgi_err("Error!","Could not allocate enough memory.");
				}
			}
			else /* if we didn't received data then inform the user and stop */
			{
				cgi_err("Error!","CONTENT_LENGTH is NULL");
			}
		}
		else /* if the request method is not supported, tell the user and stop */
		{
			cgi_err("CGI Error!","Unsupported REQUEST_METHOD");
		}

	} /* end of if statement for REQUEST_METHOD == NULL */
	else
		{
		//fprintf(stderr,"The REQUEST_METHOD environment variable\n");
		//fprintf(stderr,"does not exists.\n\n");
		//fprintf(stderr,"If you are dubugging, please set your CGI\n");
		//fprintf(stderr,"environment variables.\n\n");
		return NULL;
	} /* end of else statement for REQUEST_METHOD == NULL */


	/* this return statement will probably never be executed, it is simply
	here just to avoid warnings from some compilers.
	*/
	return NULL;

}

/*----------------------------------------------------------------------------------------*/

/*
list_print() -	This function will print out the key/value pairs in our linked list.
They will be formatted in a Definition List without any headers
being assigned.  The head pointer the user has must be passed to it.
*/
void list_print(LIST *head)
{
	printf("<DL>\n");
	TraverseList(head,Display_Entry);
	printf("</DL>\n");
}

/*----------------------------------------------------------------------------------------*/

/*
list_clear() -	The list_clear() function will clear the linked list and free up
the memory.
*/
void list_clear(LIST *head)
{
	DestroyList(head);
}

/*----------------------------------------------------------------------------------------*/

/*
list_count() -	The list_count() function will return the number of items that
exist in the linked list.
*/
long list_count(LIST *head)
{
	return(GetCount(head));
}

/*----------------------------------------------------------------------------------------*/

/*
list_delete_entry() -	This function will delete the specified entry in the
list, if NULL is supplied for the value then the first
key found will be deleted, if NULL is supplied for the
key then the first matching value will be deleted, if
both the key and value are supplied then the
key and value must match in order for the node to be
deleted.  A zero value will be returned if the
deletion was not sucessful, and a non-zero value will
be returned if it was successful.
*/
int list_delete_entry(LIST *head, char *key, char *value)
{
	CGI_LIST data;

	/* needed since some compilers have problems with non-constant initializers */
	data.key = key;
	data.value = value;

	if((DeleteEntry(head, &data, Match)) == 0)
		return FAILURE;
	else
		return SUCCESS;
}

/*----------------------------------------------------------------------------------------*/

/*
list_delete_key_multi() -	This function will delete the list of specified
entries based on the key passed.  This assumes that
multiple entries with this one named key exist.  A
zero will be returned if the deletion was not
successful, and a non-zero value will be returned
if it was successful at deleting any items,
assuming any were found.
*/
int list_delete_key_multi(LIST *head, char *key)
{
	CGI_LIST data;
	int itemsDeleted=0;

	data.key = key;
	data.value= NULL;

	while(DeleteEntry(head, &data, Match))
		itemsDeleted++;

	if(itemsDeleted > 0)
		return SUCCESS;
	else
		return FAILURE;
}

/*----------------------------------------------------------------------------------------*/

/*
list_delete_val_multi() -	This function will delete the list of specified
entries based on the value passed.	This assumes
that multiple entries with this one name value
exist.	A zero will be returned if the deletion was
not successful, and a non-zero value will be
returned if it was successuful at deleting any items,
assuming any were found.
*/
int list_delete_val_multi(LIST *head, char *value)
{
	CGI_LIST data;
	int itemsDeleted=0;

	data.key = NULL;
	data.value = value;

	while(DeleteEntry(head, &data, Match))
		itemsDeleted++;

	if(itemsDeleted > 0)
		return SUCCESS;
	else
		return FAILURE;
}

/*----------------------------------------------------------------------------------------*/

/*
find_val() -	This function will search the linked list and return the
value in the first node that it finds that matches the
specified key. If none is found then NULL is returned.
*/
char *find_val(LIST *head, char *key)
{
	CGI_LIST data;
	CGI_LIST *foundData=NULL;

	data.key = key;
	data.value = NULL;

	foundData = (CGI_LIST *)FindEntryData(head, &data, Match);

	if(foundData == NULL)
		return NULL;
	else
		return foundData->value;
}

/*----------------------------------------------------------------------------------------*/

/*
find_name() -	This function will search the linked list and return the
key in the first node that it finds that matches the
specified value. If none is found then NULL is returned.
*/
char *find_key(LIST *head, char *value)
{
	CGI_LIST data;
	CGI_LIST *foundData=NULL;

	data.key = NULL;
	data.value = value;

	foundData = (CGI_LIST *)FindEntryData(head, &data, Match);

	if(foundData == NULL)
		return NULL;
	else
		return foundData->key;
}

/*----------------------------------------------------------------------------------------*/

/*
find_val_multi() -	This function will return  the number of values found, If 0 are
returned then it means that no values were found.  The actual
values are being held in the Vals variable, in order to use this
variable the address of a **Var variable must be passed.  Space
is being allocated for Vals, so it is upto the user to free it
up when they are done.	You must pass the value to be searched for.
*/
long find_val_multi(LIST *head, char *key, char ***Vals)
{
	CGI_LIST data;
	CGI_LIST *tmpData=NULL;
	LIST *headcpy=NULL;
	entry_t *CurrentEntry=NULL;
	long found=0;

	data.key = key;
	data.value = NULL;

	headcpy = CreateList(0);

	*Vals = NULL;

	/* make a copy of starting point */
	memcpy(headcpy, head, sizeof(LIST));

	while(1)
	{
		CurrentEntry = FindEntry(headcpy, &data, Match);
		if (CurrentEntry == NULL)
			break;
		else
			{
			tmpData = (CGI_LIST *) CurrentEntry->data;
			*Vals = (char**)realloc(*Vals, sizeof(char*) * (found + 1));
			(*Vals)[found++] = tmpData->value;
			headcpy->ListHead = CurrentEntry->nextlink;

		}
	}

	free(headcpy);

	return found;

}

/*----------------------------------------------------------------------------------------*/

/*
find_key_multi() -	This function will return the number of values found.  If 0 is
returned then it means that no values were found.  The actual
values are being held in the Vals variable, in order to use this
variable the address of a **Val variable must be passed.  Space
is being allocated for Vals, so it is upto the user to free it
up when they are done.	The key to be searched for must be given.
*/
long find_key_multi(LIST *head, char *value, char ***Vals)
{
	CGI_LIST data;
	CGI_LIST *tmpData=NULL;
	LIST *headcpy=NULL;
	entry_t *CurrentEntry=NULL;
	long found=0;

	data.key = NULL;
	data.value = value;

	headcpy = CreateList(0);

	*Vals = NULL;

	/* make a copy of starting point */
	memcpy(headcpy, head, sizeof(LIST));

	while(1)
	{
		CurrentEntry = FindEntry(headcpy, &data, Match);
		if (CurrentEntry == NULL)
			break;
		else
			{
			tmpData = (CGI_LIST *) CurrentEntry->data;
			*Vals = (char**)realloc(*Vals, sizeof(char*) * (found + 1));
			(*Vals)[found++] = tmpData->key;
			headcpy->ListHead = CurrentEntry->nextlink;

		}
	}

	free(headcpy);

	return found;

}

/******************************************************************************************/
/*								   PRIVATE FUNCTIONS									  */
/******************************************************************************************/

/*----------------------------------------------------------------------------------------*/

/*
parse_data_record() -	function will take care of splitting the data into records and
breaking up those records into the key/value combination.  We
will then unescape the url and begin entering the data into the
linked list.
*/
static LIST *parse_data_record(const char *data, LIST *head)
{
	CGI_LIST record;
	char delimiter[]="&";
	char *pstr=NULL, *localData=NULL;
	int counter;
	//	int returnValue = 0;

	if((localData = (char *)calloc(sizeof(char), strlen(data) + 1)) != NULL) /* allocate data that we can modify */
	{
		strcpy(localData, data); /* copy the raw data into our variable to modify */
	}
	else /* if we can't allocate enough memory then say so */
	{
		cgi_err("Error!","Could not allocate enough memory for parsing.");
	}

	/* substitute all our + signs with spaces using our local variable */
	for(counter=0;localData[counter];counter++)
	{
		if (localData[counter] == '+')
			localData[counter]=' ';
	}


	/* the following if statement is complicated, it will go through the data that we have
	breaking it up into tokens using a delimiter, we will then pass it to the parse_data_keyvalue()
	function in order to break up into its key/value components, unescape the url and then add it to the
	linked list.
	*/
	if ((pstr = (char *)strtok(localData, delimiter)) != NULL) /* cast the return value to char */
	{
		record = parse_data_keyvalue(pstr);
		if ((InsertEntry(head,&record,sizeof(CGI_LIST),Compare)) == 0)
			cgi_err("Error!","Error adding to list!");
		while((pstr = (char *)strtok((char *)NULL,delimiter)) != NULL)
		{
			record = parse_data_keyvalue(pstr);
			if((InsertEntry(head,&record,sizeof(CGI_LIST),Compare)) == 0)
				cgi_err("Error!","Error adding to list!");
		}
	}


	return head;

}

/*----------------------------------------------------------------------------------------*/

/*
parse_data_keyvalue() - This function will parse the data into their corresponding
key/value  pairs, we will then unescape the encoded data,
and once we have done that for the key and value data we will
then add it to our linked list.
*/
static CGI_LIST parse_data_keyvalue(const char *record)
{
	CGI_LIST lrecord;
	char *key=NULL,*value=NULL, *tempStr=NULL;
	unsigned int loc=0;

	loc = (int) strcspn(record,"="); /* find the location of our delimiter */

	tempStr = (char *)calloc(sizeof(char), loc + 1); /* loc is the = which be replaced by the null character */
	strncpy(tempStr,record,loc); /* copy the key name only */
	tempStr[loc] = '\0'; /* add our terminating null char */
	key = unescape_url(tempStr);
	free(tempStr);

	if(loc != strlen(record)) /* first check that we did find an = sign, */
	{
		record = record + loc + 1; /* re-assign our pointer to go one passed the delimiter */
			
		tempStr = (char *)calloc(sizeof(char), (strlen(record) + 1)); /* need to include the 1 for null char */
		strcpy(tempStr,record); /* copy the rest of our data into the value variable */
		value = unescape_url(tempStr);
		free(tempStr);
	}
	else
	{
		value = NULL;
	}

	lrecord.key = key;	/* give the address  of the key*/
	lrecord.value = value;	 /* give the address of the value */


	return lrecord;  /* return the address of our record */

}

/*----------------------------------------------------------------------------------------*/

/*
unescape_url() -	This function will go through the strings in the key and value fields
and strip out the + and %.. characters replacing them with their
corresponding characters.
*/
static char *unescape_url(const char *string) /* use const cause we don't want to modify data */
{
	char *newString=NULL;
	int count=0;
	int numEncodedItems=0;
	char hex[3]={
		'\0'	};
	unsigned long iStr;

	numEncodedItems = num_encoded_data_items(string);

	count = 0;
	if((newString = (char *)calloc(sizeof(char), (strlen(string) + 1 - (numEncodedItems * 2)))) != NULL)
	{

		/* copy the new sring with the values decoded */
		while(string[count]) /* use the null character as a loop terminator */
		{
			if (string[count] == '%')
			{
				hex[0]=string[count+1];
				hex[1]=string[count+2];
				hex[2]='\0';
				iStr = strtoul(hex,NULL,16); /* convert to Hex char */
				newString[count]=(char)iStr;
				count++;
				string = string + 2; /* need to reset the pointer so that we don't write hex out */
			}
			else
				{
				newString[count] = string[count];
				count++;
			}
		}/* end of while loop */

		newString[count]='\0'; /* when done copying the string,need to terminate w/ null char */

	}
	else
		{
		cgi_err("Error!","Could not allocated enough memory.");
	}

	return newString; /* returning a pointer to this area */


}

/*----------------------------------------------------------------------------------------*/

/*
num_encoded_data() -	This function will figure out the number of data items that
are encoded and it will return this number.
*/
static int num_encoded_data_items(const char *encodedStr)
{
	int numItems=0;

	/*The while loop will figure out how many items we have to unescape */
	while((encodedStr = (char *)strchr(encodedStr,'%')) != NULL)
	{
		numItems++;
		encodedStr = encodedStr + 1; /* need 1 cause we need pointer to located character plus 1 */
	}

	return numItems;
}

/*----------------------------------------------------------------------------------------*/

/*
Display() - The Display() function will decide how the data should be displayed to the
user.  It will provide the formatting to be passed to the TraverseList()
function in the list-lib library.
*/
static void Display_Entry(void *data)
{
	printf("<DT><B>%s</B>\n",((CGI_LIST *)data)->key);
	printf("<DD>%s\n",((CGI_LIST *)data)->value);
}

/*----------------------------------------------------------------------------------------*/

/*	Compare() - The Compare() function will compare the values in the name and then in
the value fields to see if they are less than, equal to or greater than.
This function is beign used when inserting the data into the linked list
*/
static int Compare(void *Data1, void *Data2)
{
	int cmpValue;

	cmpValue =strcmp(((CGI_LIST *)Data1)->key, ((CGI_LIST *)Data2)->key);

	if(cmpValue < 0)
		return -1;
	else if(cmpValue > 0)
		return 1;
	else
	{
		cmpValue = strcmp(((CGI_LIST *)Data1)->value, ((CGI_LIST *)Data2)->value);
		if(cmpValue < 0)
			return -1;
		else if (cmpValue > 0)
			return 1;
		else
			return 0;
	}
}

/*----------------------------------------------------------------------------------------*/

/*
Match() -	The Match() function will return true if a match is found for the specified
key/value pair or false otherwise.
*/
static int Match(void *Data1, void *Data2)
{
	int cmpValue;



	if(((CGI_LIST *)Data1)->key == NULL)
	{
		/* if both values are NULL then they are equal */
		if(((CGI_LIST *)Data2)->value == NULL && ((CGI_LIST *)Data1)->value == NULL)
			cmpValue = 0;
		/* if only the linked list value is NULL then they are not equal */
		else if(((CGI_LIST *)Data2)->value == NULL)
			cmpValue = -2;
		/* if both values have some data then compare it */
		else
			cmpValue = strcmp(((CGI_LIST *)Data1)->value, ((CGI_LIST *)Data2)->value);

		if (cmpValue == 0)
			return 1;
		else
			return 0;
	}
	else
		{
		cmpValue = strcmp(((CGI_LIST *)Data1)->key, ((CGI_LIST *)Data2)->key);
		if(cmpValue == 0)
		{
			if(((CGI_LIST *)Data1)->value == NULL)
				return 1;
			else if((cmpValue = strcmp(((CGI_LIST *)Data1)->value, ((CGI_LIST *)Data2)->value)) == 0)
				return 1;
			else
				return 0;
		}
		else
			return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
