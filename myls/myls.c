/*************************************************************************
	> File Name: ls-l.c
	> Author:hxllhhy
	> Mail:hxllhhy@gmail.com
	> Created Time: 2017年06月30日 星期五 13时32分33秒
 ************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<linux/limits.h>
#include<dirent.h>
#include<grp.h>
#include<pwd.h>
#include<errno.h>

#define width 80       //行宽
#define PARAM_NONE 0   //无参
#define PARAM_A    1   //-a
#define PARAM_L    2   //-l

int maxlen;   //最长文件名长度(包含隐藏文件)
int maxlen2;  //最长文件名长度(不包含隐藏文件)
int leavelen=width;   //行剩余长度
int count;  //文件总数


int part_param(int argc,char **argvv);   //解析参数
void get_dirinfo(int param,char *path);  //得到目录信息
void sort_pathname(char pathname[256][PATH_MAX+1]);  //排序文件名
void deal_filename(int param,char *fullpathname);  //处理完整路径文件名
void do_param(int param,char *name,char *fullpathname);  //根据参数调用函数
void show_fileinfo(struct stat st,char *name);  //打印文件信息
void show_singlefile(int param,char *name);  //打印单个文件名 
void my_err(const char *err_string,int line);  //错误处理函数


//错误处理函数，打印出错误信息和错误所在行
void my_err(const char *err_string,int line) 
{
    fprintf(stderr,"line:%d ",line);
    perror(err_string);
    exit(1);
}

//获取文件的属性并打印
void show_fileinfo(struct stat st,char *name)
{
    
    struct passwd *psd;  //获取文件username
    struct group *grp;   //获取文件groupname
    char time[32];  //存储时间的字符串数组
    char str[11]={"----------"};  
 
    
    //获取文件类型
    if(S_ISLNK(st.st_mode))   str[0]='l';
    if(S_ISDIR(st.st_mode))   str[0]='d';
    if(S_ISCHR(st.st_mode))   str[0]='c';
    if(S_ISBLK(st.st_mode))   str[0]='b';
    if(S_ISFIFO(st.st_mode))  str[0]='f';
    if(S_ISSOCK(st.st_mode))  str[0]='s';

    //获取文件u,g,o权限
    if(S_IRUSR & st.st_mode)  str[1]='r';
    if(S_IWUSR & st.st_mode)  str[2]='w';
    if(S_IXUSR & st.st_mode)  str[3]='x';
    if(S_IRGRP & st.st_mode)  str[4]='r';
    if(S_IWGRP & st.st_mode)  str[5]='w';
    if(S_IXGRP & st.st_mode)  str[6]='x';
    if(S_IROTH & st.st_mode)  str[7]='r';
    if(S_IWOTH & st.st_mode)  str[8]='w';
    if(S_IXOTH & st.st_mode)  str[9]='x';

    //获取用户名，组名
    psd=getpwuid(st.st_uid);
    grp=getgrgid(st.st_gid);

    strcpy(time,ctime(&st.st_mtime));  //将格林位置时间转化成正常格式
    time[strlen(time)-1]='\0';         //把字符串尾的\n换成\0
    
    printf("%s %4lu %-8s %-8s %-6ld %-s %-s\n",str,st.st_nlink,psd->pw_name,grp->gr_name,st.st_size,time,name);
}

//打印单个文件名，并对齐
void show_singlefile(int maxlenth,char *name)
{
    int i,len,blank;
    if(leavelen<maxlenth)  //该行不够打印一个文件名了，换行
    { 
        printf("\n");
        leavelen=width;
    }
    len=strlen(name);
    blank=maxlenth-len;   //在最长文件名长度范围内把剩余的长度补上空格
    printf("%s",name);
    while(blank--)
    {
        printf(" ");
    }
    printf("  ");
    leavelen-=(maxlenth+2);   //该行剩余长度
}

//显示目录下文件信息
void get_dirinfo(int param,char *path)
{
    DIR *dir;
    struct dirent *ptr;
    char pathname[256][PATH_MAX+1],fullpathname[256][PATH_MAX+1];
    int i,j;

    //获取目录下的文件名，文件总数和最长文件名长度，文件名存在数组pathname中
    dir=opendir(path); // 打开目录，返回DIR*形态的目录流
    if(dir==NULL)
        my_err("opendir",__LINE__);

    while((ptr=readdir(dir))!=NULL)   //读取目录项信息，返回struct dirent结构的指针
    {
        if(ptr==NULL)
            my_err("readdir",__LINE__);

        strcpy(pathname[count],ptr->d_name);  //把文件名存入pathname数组里
        pathname[count][strlen(ptr->d_name)]='\0';

        if(strlen(ptr->d_name)>maxlen)  //得到最长文件名长度
        {
            maxlen=strlen(ptr->d_name);
        }
        if(pathname[count][0]!='.')
            if(strlen(ptr->d_name)>maxlen2)
                maxlen2=strlen(ptr->d_name);

        count++;
    }

    if(count>256)
        my_err("too many files under this dir",__LINE__);

    sort_pathname(pathname);  //文件名排序

    for(i=0;i<count;i++)
    {
        strcpy(fullpathname[i],path);
        strcat(fullpathname[i],pathname[i]);   //补全文件名，得到完整路径文件名，存入fullpathname数组里
        deal_filename(param,fullpathname[i]);  //调用
    }

    closedir(dir);   //关闭文件
}

//对文件名进行排序，冒泡
void sort_pathname(char pathname[256][PATH_MAX+1])
{
    int i,j;
    char t[PATH_MAX+1];

    for(i=0;i<count;i++)
        for(j=0;j<count-i-1;j++)
            if(strcmp(pathname[j],pathname[j+1])>0)
            {
                strcpy(t,pathname[j+1]);
                t[strlen(pathname[j+1])]='\0';
                strcpy(pathname[j+1],pathname[j]);
                pathname[j+1][strlen(pathname[j])]='\0';
                strcpy(pathname[j],t);
                pathname[j][strlen(t)]='\0';
            }
}

//用完整路径文件名获取文件属性，再处理完整路径文件名解析出文件名
void deal_filename(int param,char *fullpathname)
{
    int i,j,mark=0,k=0;
    char name[NAME_MAX+1];

    for(i=strlen(fullpathname)-1;i>0;i--)  //从后向前遍历
        if(fullpathname[i-1]=='/')  //遇到的第一个/，标记下标
        {
            mark=i;
            break;
        }
    for(j=mark;fullpathname[j]!='\0';j++)
    {
         name[k]=fullpathname[j];  //将下标从标记到尾的字符存入数组name里
         k++;
    }
    name[k]='\0';

    do_param(param,name,fullpathname);
}

//根据传入的参数信息，调用相应的函数
void do_param(int param,char *name,char *fullpathname)
{
    struct stat st;
    if(lstat(fullpathname,&st)==-1)
    {
        printf("fullpathname=%s\n",fullpathname);
        my_err("stat",__LINE__);
    } 

    switch(param)
    {
        case PARAM_NONE:
                if(name[0]!='.')
                    show_singlefile(maxlen2,name);
                break;
        case PARAM_A:
                show_singlefile(maxlen,name);
                break;
        case PARAM_L:
                if(name[0]!='.')
                    show_fileinfo(st,name);
                break;
        case PARAM_A+PARAM_L:
                show_fileinfo(st,name);
                break;
        default:
                break;        
    }   
}

//解析参数，得到一个整型的返回值
int part_param(int argcc,char **argvv)
{
    int flag;
    if(argcc==1)
        flag=PARAM_NONE;
    if(argcc==2)
    {
        if(strcmp(argvv[1],"-a")==0)
            flag=PARAM_A;
        else if(strcmp(argvv[1],"-l")==0)
            flag=PARAM_L;
        else if((strcmp(argvv[1],"-al")==0)||(strcmp(argvv[1],"-la")==0))
            flag=PARAM_A+PARAM_L;            
    }
    if(argcc==3)
    {
        if((strcmp(argvv[1],"-a")==0)&&(strcmp(argvv[2],"-l")==0))
            flag=PARAM_A+PARAM_L;
        else if((strcmp(argvv[1],"-l")==0)&&(strcmp(argvv[2],"-a")==0))
            flag=PARAM_A+PARAM_L;
    }
    return flag; 
}

int main(int argc,char **argv)
{
    int i,param;
    int num=0;
    struct stat st;
    char path[PATH_MAX];

    for(i=1;i<argc;i++)
    {
        if(argv[i][0]=='-')
            num++;  //保存-的个数
    }


    if((num+1)==argc)  //不输入文件名或目录
    {
        strcpy(path,"./");
        path[2]='\0';
        param=part_param(argc,argv);
        get_dirinfo(param,path);
        printf("\n");
        return 0;   
    }

    i=1;
    do{
        param=part_param(num+1,argv);  //得到参数信息
        if(argv[i][0]=='-')  //寻找文件或目录，是参数就continue
        {
            i++;
            continue;
        }
        else
        {
            strcpy(path,argv[i]);

            if(stat(path,&st)==-1)   //有错误，目标文件和目录不存在
                my_err("stat",__LINE__);

            if(S_ISDIR(st.st_mode))  //argv[i]是个目录
            {
                if(path[strlen(argv[i])-1]!='/')
                {
                    path[strlen(argv[i])]='/';
                    path[strlen(argv[i])+1]='\0';
                }
                else
                    path[strlen(argv[i])]='\0';

                get_dirinfo(param,path);
                printf("\n");
                count=0;
                i++;
            }
            else  //argv[i]是个文件
            {
                deal_filename(param,path);
                i++;
            }
        }
    }while(i<argc);
    return 0;
}
