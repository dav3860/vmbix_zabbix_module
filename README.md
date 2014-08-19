zbx_vmbix
=====================

Description
-----------
Zabbix 2.2 comes with support of loadable modules for extending Zabbix agent and server without sacrificing performance.

A loadable module is basically a shared library used by Zabbix server or agent and loaded on startup. The library should contain certain functions, so that a Zabbix process may detect that the file is indeed a module it can load and work with.

Loadable modules have a number of benefits. Great performance and ability to implement any logic are very important, but perhaps the most important advantage is the ability to develop, use and share Zabbix modules. It contributes to trouble-free maintenance and helps to deliver new functionality easier and independently of the Zabbix core code base.

I have created a module to query the [VmBix](https://github.com/dav3860/vmbix) daemon, used to monitor a VMWare infrastructure with Zabbix.

Install
-------

From **source**:

You need to download the Zabbix 2.2.x source and:

```
cd <source_zabbix>
./configure
make

cd <source_zabbix>/src/modules/ 
```

and you should create a new directory with this git repo content. After that, inside of the new module directory, a `make` is enough. This will create the zbx_vmbix.so file.

**Other**: There is a compiled version too. Copy it wherever you want (for example in /usr/lib/zabbix/modules).

Configure
---------

**For an agent module:** `cd /etc/zabbix/zabbix_agentd.conf`

**For a server module:** `cd /etc/zabbix/zabbix_server.conf`

```
LoadModulePath=/usr/lib/zabbix/modules
LoadModule=zbx_vmbix.so
```

And restart the agent or server.

By default, the module will query VmBix on localhost and port 12050. You can create a configuration file /etc/zabbix/vmbix_module.conf in you want to change this. A sample configuration file is provided.

You can test it like this with a Zabbix agent for example :

```
[TEST zbx_vmbix]# zabbix_agentd -t vmbix[vm.guest.os,VM01]
vmbix[vm.guest.os,VM01]                  [s|Red Hat Enterprise Linux 6 (64 bits)]
```

Zabbix 2.2.x template
---------------------

Some templates will be provided.
