#include <pthread.h>
#include "sys/time.h"
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#define QUEUESIZE 1		
#define CHARLEN 1035
int keepRunning = 2; //variable used for manipulating signals
sem_t semvar; //semaphore to sleep main and decrease cpu usage
clock_t start_t, end_t, total_t; // variables for cpu clocks measurement

void *wifi_scan (void *args);//producer
void *wifi_store (void *args);//consumer

typedef struct
{
    int buf[QUEUESIZE];
    long head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
    char filename[1124];
    struct timeval times, tempTime;
    double timestamp, interval;//interval between two scans // timestamp need?
    char **charArray; // array with ssids values filled by wifi_scan
    int arrayLen, counterFile; 
} queue;

queue *queueInit (double inter);
void queueDelete (queue * q);
void queueAdd (queue * q, int in);
void queueDel (queue * q, int *out);
void intHandler ();
char *timeStamp (char *result, struct timeval tv);//function returning formatted date-time
int
main (int argc, char *argv[])
{
    if (argc != 2)
    {
        printf ("interval\n"
                "where\n" "interval between 2 samples (seconds) \n", argv[0]);
        return (1);
    }
    double interval = atof (argv[1]);//interval as double

    FILE *fp;
    char path[CHARLEN];
    signal (SIGINT, intHandler);//handle signal (SIGINT) Ctrl+c
    sem_init(&semvar, 0, 0); //initialize semaphore
    queue *fifo; //queue to handle wifi_scan and wifi store - defined as 1 (default) - functions for handling queue maintained
    pthread_t scan, store;

    fifo = queueInit (interval);
    if (fifo == NULL)
    {
        fprintf (stderr, "main: Queue Init failed.\n");
        exit (1);
    }
    struct timeval startTimeClock, endTimeClock;
    gettimeofday (&startTimeClock, NULL);
    start_t = clock(); //functions for cpu usage measurement
    //printf("Starting of the program, start_t = %ld\n", start_t);
    pthread_create (&scan, NULL, wifi_scan, fifo);
    pthread_create (&store, NULL, wifi_store, fifo);
	printf("\nProgram is running... Press Ctrl+C to end execution");
	sem_wait(&semvar);
    pthread_join (scan, NULL);
    pthread_join (store, NULL);
    queueDelete (fifo);
    sem_destroy(&semvar);
    end_t = clock();
    gettimeofday (&endTimeClock, NULL);
    double timeClock =(endTimeClock.tv_sec - startTimeClock.tv_sec) + (endTimeClock.tv_usec -startTimeClock.tv_usec) /1.0e6;
    printf("\nTime elapsed= %f",timeClock);    
    //printf("\nEnd of the big loop, end_t = %ld", end_t);
    double total_t1 = (double)(end_t - start_t) / CLOCKS_PER_SEC;
    printf("\nTotal time taken by CPU: %f\n", total_t1  );
    
    return 0;
}

void *wifi_scan (void *q)
{
    queue *fifo;

    fifo = (queue *) q;
    FILE *fp, *tem;
    char path[1035];

    int j = 0, i = 0;
    struct timeval startTime, endTime;	//two timeval structs for measuring time
    long long int expectedTime = fifo->interval * 1000000;	//convert to micro seconds , expectedTime is used to regulate interval in order to keep stable the offset(time difference)
    double adapter = 0;		//offset taken from previous measurement in order to add/subtract it from the next sleep
    long long int temp = 0;
    int counter = 0;

    gettimeofday (&startTime, NULL);
    for (;;)
    {

        pthread_mutex_lock (fifo->mut);
        while (fifo->full)
        {
            pthread_cond_wait (fifo->notFull, fifo->mut);
        }

        while ((fp = popen ("/bin/sh searchWifi.sh", "r")) == NULL){
             printf ("Failed to run command\n");
             if(keepRunning==1 || keepRunning==0){
				 fifo->arrayLen = 0;
				 printf("\nexiting after error occured");
				 exit(-1);

			  }

           
        }

        fifo->arrayLen = 0;
        
		//save ssids to array in order to pass them to wifi_store
        j = 0;
        int tempor = 1;
        fifo->charArray = malloc (tempor * sizeof (char *));
        while (fgets (path, sizeof (path) - 1, fp) != NULL)
        {
            tempor++;
            fifo->charArray = (char *) realloc (fifo->charArray, tempor * sizeof (*(fifo->charArray)));	//allocate memory and save ssids to charArray
            fifo->charArray[j] = malloc ((CHARLEN + 1) * sizeof (char));
            strcpy (fifo->charArray[j], path);
            j++;
        }
        fifo->arrayLen = j;//number of ssids found

        pclose (fp);
        gettimeofday (&(fifo->times), NULL);//timestamp to measure time between wifi scan and save to file 

        if (i == 0)//execute only first time
        {
            gettimeofday (&endTime, NULL);
            temp =1000000 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec -startTime.tv_usec);
            adapter = temp;
            expectedTime = fifo->interval * 1000000 - adapter;
            if (((fifo->interval) * 1000000 - adapter) < 0)
                expectedTime = 0;
        }
        queueAdd (fifo, i);//function for handling queue - in order to be extensible
        pthread_mutex_unlock (fifo->mut);
        pthread_cond_signal (fifo->notEmpty);

        usleep (expectedTime);//sleep thread in order to wait for the next scan
        gettimeofday (&endTime, NULL);
        temp =1000000 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec -startTime.tv_usec);
        double pr =(endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec -startTime.tv_usec) / 1.0e6;

		//save real interval between 2 scans
        tem = fopen ("timesStopwatch", "a");
        if (!tem)
        {
            perror ("Could not open the file3");
            exit (0);
        }
        fprintf (tem, "%f\n", pr);
        fclose (tem);
        //calculate remaining time (interval - overhead) for the next sleep 
        adapter = temp - expectedTime;
        expectedTime = fifo->interval * 1000000 - adapter;
        if (((fifo->interval) * 1000000 - adapter) < 0 && (fifo->interval < 2))
            expectedTime = 0;	//proxeiro
        if (((fifo->interval) * 1000000 - adapter) < 0)
            expectedTime = 0;
        startTime = endTime;//set next time to include calculations

        i++;
        if (keepRunning == 0)//check condition if ctrl+c was pressed pthread_exit
        {
            printf ("\nExited1");
            break;
        }
    }
    pthread_exit(NULL);
}

void remove_newline_ch (char *line)//function for removing \n 
{
    int new_line = strlen (line) - 1;
    if (line[new_line] == '\n')
        line[new_line] = '\0';
}

void *wifi_store (void *q) //function in order to save ssids to file
{
    queue *fifo;
    int i, d;

    fifo = (queue *) q;
    while (1)
    {
        pthread_mutex_lock (fifo->mut);

        while (fifo->empty)
        {
            pthread_cond_wait (fifo->notEmpty, fifo->mut);
        }
        queueDel (fifo, &d);

        struct timeval timeFile;
        char tempPath[1035];
        FILE *ftemp, *ftemp2;
        char line[1035];
        char timestamp[1024];

        int flag = 0, k, flag2 = 0;
        char ch;
        if (fifo->counterFile >= 2)//check if two times saving to file takes > interval create new file with old name+timestamp
        {
            gettimeofday (&timeFile, NULL);
            memset (timestamp, 0, 1024);	/*all bytes are set to '\0' */
            timeStamp (timestamp, timeFile);
            memset (fifo->filename, 0, 1124);
            strcpy (fifo->filename, "ssids");
            strcat (fifo->filename, timestamp);
            fifo->counterFile = 0;
        }

        //calculate timestamp
        memset (timestamp, 0, 1024);	/*all bytes are set to '\0' */
		timeStamp (timestamp, fifo->times);
        gettimeofday (&timeFile, NULL);
        
        for (k = 0; k < fifo->arrayLen; k++)//loop to save each ssid
        {   //main loop
            flag = 0;
            flag2 = 0;
            strcpy (tempPath, fifo->charArray[k]);//copy ssid to tempPath

            ftemp = fopen (fifo->filename, "r");
            if (!ftemp)
            {
                ftemp = fopen (fifo->filename, "w+");//if file doesn't exist create new
                if (!ftemp)
                {
                    perror ("Could not open the file2");
                    exit (0);
                }
            }
            int chars = 0;
            char *token_write, *token_line;
            const char s[] = "\"";//delimiters
            const char s2[] = "\n";
            const char s3[] = " ";
            char buff[1035];

            token_write = strtok (tempPath, s);//remove "" from ssid name
            token_write = strtok (NULL, s);
            remove_newline_ch (token_write);// remove newline character
            int n = 0, result;
            char *buf;
            int count = 0;

            char *search_buf = (char *) malloc (sizeof (char) * (strlen (token_write) + 1));//buffer for searching file for ssid
            int sum = 0;
            int g = 0;
            int l = 0;
			//sum keeps number of bytes-characters from the begin to the position where new timestamp will be placed 
            while (fgets (search_buf, strlen (token_write) + 1, ftemp) != NULL)//scan line by line
            {   

                sum += (strlen (search_buf));
                if (flag == 1 && ((strstr (search_buf, "\n")) != NULL))//if ssid is found wait until newline character is read
                {
                    break;
                }

                if (strcmp (search_buf, token_write) == 0)//flag indicates that ssid is found
                {
                    flag = 1;

                }

                l++;
            }
            free(search_buf);
            int lNumber = l;
            //to sum exei arithmo byte mexri na brw

            fclose (ftemp);
            if (flag == 0) //when ssid doesn't exist add name and timestamp at the end of the file
            {
                //append at the end of file
                ftemp = fopen (fifo->filename, "a");	//kanonika r
                fwrite (token_write, sizeof (char), strlen (token_write),
                        ftemp);
                //fwrite(s3,sizeof(char),strlen(s3),ftemp);
                fwrite (timestamp, sizeof (char), strlen (timestamp), ftemp);
                fwrite (s2, sizeof (char), strlen (s2), ftemp);//add newline character
                fclose (ftemp);
                gettimeofday (&(fifo->tempTime), NULL);//time when ssid is saved to file
            }
            else
            {
                ftemp = fopen (fifo->filename, "r+");
                fseek (ftemp, 0L, SEEK_END);//go at the end of file and find the size of it
                int sz = ftell (ftemp);

				if(sz>=150000){//if file is bigger than 150kb set counterFile to create new
					fifo->counterFile=2;
				}
                fseek (ftemp, sum, SEEK_SET);//open file from the position of sum
                int tem = sz - sum;//keep bytes after the place where the timestamp will be saved
                if ((sz - sum) == 0)
                {
                    tem = 1;
                }

                char *c = (char *) malloc (sizeof (char) * (tem));//allocate memory for the rest of file and read it to buffer
                if (tem != 1)
                    fread (c, sizeof (char), tem, ftemp);

                fseek (ftemp, sum - strlen (s2), SEEK_SET);//go to the position(target) - "\n" char and save timestamp 
                //fwrite(s3,sizeof(char),strlen(s3),ftemp);
                fwrite (timestamp, sizeof (char), strlen (timestamp), ftemp);
                fwrite (s2, sizeof (char), strlen (s2), ftemp);//add newline
				//care if it is at the end
                if (tem != 1)
                    fwrite (c, sizeof (char), (tem), ftemp);
                free (c);

                fclose (ftemp);

                gettimeofday (&(fifo->tempTime), NULL);//time when ssid is saved to file

            }
            //calculate time between scan and save
            double tempTimeD =(fifo->tempTime.tv_sec - fifo->times.tv_sec) + (fifo->tempTime.tv_usec - fifo->times.tv_usec) / 1.0e6;
            ftemp = fopen ("times", "a");//save time until the ssids is saved
            if (!ftemp)
            {
                perror ("Could not open the file3");
                exit (0);
            }
            fprintf (ftemp, "%f\n", tempTimeD);
            fclose (ftemp);

        }
		if(fifo->arrayLen==0)gettimeofday (&(fifo->tempTime), NULL);
        double tempTimeFile =(fifo->tempTime.tv_sec - timeFile.tv_sec) + (fifo->tempTime.tv_usec -timeFile.tv_usec) /1.0e6;
        if (tempTimeFile > fifo->interval)//timestamp measure difference in order to decide if it will create new file for ssids
        {
            fifo->counterFile++;
        }
        int t=fifo->arrayLen-1;
        while(t>=0) {
            free(fifo->charArray[t]);
            t--;
        }
        free(fifo->charArray);

        pthread_mutex_unlock (fifo->mut);
        pthread_cond_signal (fifo->notFull);
        if (keepRunning == 1)//check if ctrl+ c is pressed and exit only after saving files
        {
            keepRunning--;
            printf ("\nExited2");
            break;
        }
    }
    pthread_exit(NULL);
}

queue *queueInit (double inter)
{
    queue *q;

    q = (queue *) malloc (sizeof (queue));
    if (q == NULL)
        return (NULL);

    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    q->interval = inter;
    q->counterFile = 0;
    strcpy (q->filename, "ssids");
    q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
    pthread_mutex_init (q->mut, NULL);
    q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notFull, NULL);
    q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notEmpty, NULL);

    return (q);
}

void queueDelete (queue * q)
{
    pthread_mutex_destroy (q->mut);
    free (q->mut);
    pthread_cond_destroy (q->notFull);
    free (q->notFull);
    pthread_cond_destroy (q->notEmpty);
    free (q->notEmpty);
    free (q);
}

void queueAdd (queue * q, int in)
{
    q->buf[q->tail] = in;
    q->tail++;
    if (q->tail == QUEUESIZE)
        q->tail = 0;
    if (q->tail == q->head)
        q->full = 1;
    q->empty = 0;

    return;
}

void queueDel (queue * q, int *out)
{
    *out = q->buf[q->head];

    q->head++;
    if (q->head == QUEUESIZE)
        q->head = 0;
    if (q->head == q->tail)
        q->empty = 1;
    q->full = 0;

    return;
}

void intHandler ()
{
	keepRunning=1;
	sem_post(&semvar);
}

char *timeStamp (char *result, struct timeval tv)//format timestamp
{
    long milliseconds = (tv.tv_sec * 1000L) + tv.tv_usec / 1000L;
    long microseconds = (tv.tv_sec * 1000000L) + tv.tv_usec;
    int days = tv.tv_sec / (60 * 60 * 24);
    time_t nowtime = tv.tv_sec;
    struct tm *nowtm = localtime (&nowtime);
    char buf1[1024];
    char buf2[1024];
    strftime (buf1, 1024, " %Y-%m-%d %H:%M:%S", nowtm);
    // append microseconds to end of string
    snprintf (buf2, 1024, "%s.%06ld", buf1, tv.tv_usec);
    strcpy (result, buf2);
    return result;
}

