zbx_vmbix
=====================

Description
-----------
Zabbix 2.2 comes with support of loadable modules for extending Zabbix agent and server without sacrificing performance.

A loadable module is basically a shared library used by Zabbix server or agent and loaded on startup. The library should contain certain functions, so that a Zabbix process may detect that the file is indeed a module it can load and work with.

I have created a module to query the [VmBix](https://github.com/dav3860/vmbix) daemon, used to monitor a VMWare infrastructure with Zabbix.

Install
-------

From **source**:

You need to download the Zabbix source (2.2 or 2.4) configure the environment :

```
cd <source_zabbix>
./configure

cd <source_zabbix>/src/modules/ 
```

And you should create a new directory with this git repo content :
```
git clone https://github.com/dav3860/vmbix_zabbix_module.git
cd vmbix_zabbix_module
```

After that, compile the module :

### For Zabbix 2.2 :
```
make vmbix-2.2
```

### For Zabbix 2.4 :
```
make vmbix-2.4
```

### For Zabbix 3.0 :
```
make vmbix-3.0
```

This will create the vmbix.so file. Put it into a /usr/lib/zabbix/modules directory for example.

**binary**:
A compiled module is provided too (for Zabbix 2.4, tested on Centos 6 x64).

Configure
---------

**For an agent :** `cd /etc/zabbix/zabbix_agentd.conf`

**For a server :** `cd /etc/zabbix/zabbix_server.conf`

**For a proxy :** `cd /etc/zabbix/zabbix_proxy.conf`

```
LoadModulePath=/usr/lib/zabbix/modules
LoadModule=vmbix.so
```

And restart the agent or server.

By default, the module will query VmBix on localhost and port 12050. You can create a configuration file /etc/zabbix/vmbix_module.conf if you want to change this. A sample configuration file is provided.

You can test it like this with a Zabbix agent for example :

```
[TEST zbx_vmbix]# zabbix_agentd -t vmbix[vm.guest.os,VM01]
vmbix[vm.guest.os,VM01]                  [s|Red Hat Enterprise Linux 6 (64 bits)]
```

Zabbix 2.x templates
---------------------

Sample templates using this loadable module are provided in the [VmBix repository](https://github.com/dav3860/vmbix/tree/master/src/zabbix_templates).
