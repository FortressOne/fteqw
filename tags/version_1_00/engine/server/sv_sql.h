#ifndef MYSQLDLL_H
#define MYSQLDLL_H
#include <mysql/mysql.h>

dllhandle_t *mysqlhandle;

#define SQL_CONNECT_STRUCTPARAMS 2
#define SQL_CONNECT_PARAMS 4

typedef enum 
{
	SQLDRV_MYSQL,
	SQLDRV_SQLITE, /* NOT IN YET */
	SQLDRV_INVALID
} sqldrv_t;

typedef struct queryrequest_s
{
	int num; /* query number reference */
	qboolean persistant; /* persistant query */
	struct queryrequest_s *next; /* next request in queue */
	int callback; /* callback function reference */
	int selfent; /* self entity on call */
	float selfid; /* self entity id on call */
	int otherent; /* other entity on call */
	float otherid; /* other entity id on call */
	char query[1]; /* query to run */
} queryrequest_t;

typedef struct queryresult_s
{
	struct queryrequest_s *request; /* corresponding request */
	struct queryresult_s *next; /* next result in queue */
	int rows; /* rows contained in single result set */
	int columns; /* fields */
	qboolean eof; /* end of query reached */
	MYSQL_RES *result; /* result set from mysql */
#if 0
	char **resultset; /* stored result set from partial fetch */
#endif
	char error[1]; /* error string, "" if none */
} queryresult_t;

typedef struct sqlserver_s
{
	void *thread; /* worker thread for server */
	sqldrv_t driver; /* driver type */
	MYSQL *mysql; /* mysql server */
	volatile qboolean active; /* set to false to kill thread */
	void *requestcondv; /* lock and conditional variable for queue read/write */
	void *resultlock; /* mutex for queue read/write */
	int querynum; /* next reference number for queries */
	queryrequest_t *requests; /* query requests queue */
	queryrequest_t *requestslast; /* query requests queue last link */
	queryresult_t *results; /* query results queue */
	queryresult_t *resultslast; /* query results queue last link */
	queryresult_t *currentresult; /* current called result */
	queryresult_t *persistresults; /* list of persistant results */
	queryresult_t *serverresult; /* server error results */
	char **connectparams; /* connect parameters (0 = host, 1 = user, 2 = pass, 3 = defaultdb) */
} sqlserver_t;

/* prototypes */
void SQL_Init(void);
void SQL_KillServers(void);
void SQL_DeInit(void);

sqlserver_t *SQL_GetServer (int serveridx, qboolean inactives);
queryresult_t *SQL_GetQueryResult (sqlserver_t *server, int queryidx);
void SQL_DeallocResult(queryresult_t *qres);
void SQL_ClosePersistantResult(sqlserver_t *server, queryresult_t *qres);
void SQL_CloseResult(sqlserver_t *server, queryresult_t *qres);
void SQL_CloseAllResults(sqlserver_t *server);
char *SQL_ReadField (sqlserver_t *server, queryresult_t *qres, int row, int col, qboolean fields);
int SQL_NewServer(char *driver, char **paramstr);
int SQL_NewQuery(sqlserver_t *server, int callfunc, int type, int self, float selfid, int other, float otherid, char *str);
void SQL_Disconnect(sqlserver_t *server);
void SQL_Escape(sqlserver_t *server, char *src, char *dst, int dstlen);
char *SQL_Info(sqlserver_t *server);
qboolean SQL_Available(void);
void SQL_ServerCycle (progfuncs_t *prinst, struct globalvars_s *pr_globals);

extern cvar_t sql_driver;
extern cvar_t sql_host;
extern cvar_t sql_username;
extern cvar_t sql_password;
extern cvar_t sql_defaultdb;

#define SQLCVAROPTIONS "SQL Defaults"

#endif