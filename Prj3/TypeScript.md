#  TypeScript
##  Step1
Use the following command to build:
```c
make
```
Start with
```c
./disk <cylinders> <sector per cylinder> <track-to-track delay> <disk-storage filename>
```
For example, use
```c
./disk 3 4 20 file_storage 
```
Then use command:
```c    
I
R 2 3
W 2 3 it is a test file
R 2 3
W 5 6
(response:Insructions error!)
R 5 6
(response:Insructions error!)
E
```
disk.log will be:
```c
3 4
YES
YES
YES it is a test file
NO
NO
Goodbye
```

## Step2
Use the following command to build:
```c
make
```
Start with
```c
./fs
```
Then use command:
```c    
f
    Done
mkdir testDir1
    Yes
mkdir testDir2
    Yes
cd testDir1
    Yes
mk testf1
    Yes
mk testf2
    Yes
ls
    Yes
    FILE testf1                  0 bytes  mode 0000  mtime 1970-01-01 08:00:00
    FILE testf2                  0 bytes  mode 0000  mtime 1970-01-01 08:00:00
w testf1 20 it is a test file!
    Yes
cat testf1
    Yes
    it is a test file!
i testf1 15 17 insert something
    Yes
cat testf1
    Yes
it is a test fiinsert somethingle!
rm testf1
    Yes
ls
    Yes
    FILE testf2                  0 bytes  mode 0000  mtime 1970-01-01 08:00:00
cd ..
    Yes
rmdir testDir2
    Yes
ls
    Yes
    DIR  testDir1               96 bytes  mode 0111  mtime 2025-05-21 10:46:55
e
    Bye!
```
The tab used here in the output is only to help readers clearly distinguish between commands and responses; in actual implementation, there is no such tab.
`fs.log` will be:
```c    
[INFO] Use command: f
[INFO] Success
[INFO] Use command: mkdir testDir1
[INFO] Success
[INFO] Use command: mkdir testDir2
[INFO] Success
[INFO] Use command: cd testDir1
[INFO] Success
[INFO] Use command: mk testf1
[INFO] Success
[INFO] Use command: mk testf2
[INFO] Success
[INFO] Use command: ls
[INFO] Success
[INFO] Use command: w testf1 20 it is a test file!
[INFO] Success
[INFO] Use command: cat testf1
[INFO] Success
[INFO] Use command: i testf1 15 17 insert something
[INFO] Success
[INFO] Use command: cat testf1
[INFO] Success
[INFO] Use command: rm testf1
[INFO] Success
[INFO] Use command: ls
[INFO] Success
[INFO] Use command: cd ..
[INFO] Success
[INFO] Use command: rmdir testDir2
[INFO] Success
[INFO] Use command: ls
[INFO] Success
[INFO] Use command: e
[INFO] Exit
```
## Step3
Open 4 terminals, and enter the following commands respectively:
```c  
./disk 5 4  10 disk_storage 8071
```
```c  
./fs 8071 8072
```
```c  
./client 8072
```
```c  
./client 8072
```

In one client, input:
```c
> login 0
    Yes
> f
    Done
> mk a
    Yes
> mkdir mydir
    Yes
> ls
    Yes
    FILE a                       0 bytes  mode 0700  mtime 2025-05-21 19:42:33  owner 0
    DIR  mydir                  64 bytes  mode 0701  mtime 2025-05-21 19:42:40  owner 0
> cd mydir
    Yes
> mk b
    Yes
> ls
    Yes
    FILE b                       0 bytes  mode 0700  mtime 2025-05-21 19:43:11  owner 0
> cd ..
    Yes
> ls
    Yes
    FILE a                       0 bytes  mode 0700  mtime 2025-05-21 19:42:33  owner 0
    DIR  mydir                  96 bytes  mode 0701  mtime 2025-05-21 19:43:11  owner 0
> w a 12 hello_world!
    Yes
> i a 2 4 file
    Yes
> cat a
    Yes
    hefilello_world!
> d a 2 4 
    Yes
> cat a
    Yes
    hello_world!
```
In the other client, input:
```c    
> login 2
    Yes
> mk b
    Yes
> cat a
    No
    Failed to read file
> w b 4 file
    Yes
> cd mydir
    No
    Failed to change directory
```
Switch back to the client logged in as uid 0:
```c
> cat b
    Yes
    file
> rm a
    Yes
> rmdir mydir
    Yes
> ls
    Yes
    FILE b                       0 bytes  mode 0700  mtime 2025-05-21 19:44:15  owner 2      
> e
    Bye!
>
    recv: Success
```