# Zabbix VmBix Module ![alt text](https://travis-ci.org/dav3860/vmbix_zabbix_module.svg?branch=master "Build Status")

## Description
Zabbix 2.2+ comes with support of loadable modules for extending Zabbix agent and server without sacrificing performance.

A loadable module is basically a shared library used by Zabbix server or agent and loaded on startup. The library should contain certain functions, so that a Zabbix process may detect that the file is indeed a module it can load and work with.

I have created a module to query the [VmBix](https://github.com/dav3860/vmbix) daemon, used to monitor a VMWare infrastructure with Zabbix.

## Install
Get the latest version [here](https://bintray.com/dav3860/generic/vmbix-zabbix-module/view#files)
By default, the packages will create a /etc/zabbix/zabbix_agentd.d/modules.conf file to enable the module. Configure it for the server or the proxy if needed (see the Configure section).

## Or build from source
You need to download the Zabbix source and configure the environment :

```
cd <source_zabbix>
```
For Zabbix 3+ :
```
./configure --with-openssl --enable-ipv6
```
Then :
```
cd <source_zabbix>/src/modules/
```

And you should create a new directory with this git repo content :
```
git clone https://github.com/dav3860/vmbix_zabbix_module.git
cd vmbix_zabbix_module
```

After that, compile the module :

### For Zabbix 3.4 :
```
make vmbix-3.4
```

### For Zabbix 3.2 :
```
make vmbix-3.2
```

### For Zabbix 3.0 :
```
make vmbix-3.0
```

This will create the vmbix.so file. Put it into a /usr/lib/zabbix/modules directory for example.

## Configure

**For an agent :** `/etc/zabbix/zabbix_agentd.conf`

**For a server :** `/etc/zabbix/zabbix_server.conf`

**For a proxy :** `/etc/zabbix/zabbix_proxy.conf`

```
LoadModulePath=/usr/lib/zabbix/modules
LoadModule=vmbix.so
```

By default, the packages will create a /etc/zabbix/zabbix_agentd.d/modules.conf file with parameters above.

And restart the agent or server/proxy.

By default, the module will query VmBix on localhost and port 12050. You can create a configuration file /etc/zabbix/vmbix_module.conf if you want to change this. A sample configuration file is provided.

You can test it like this for example if you configured a Zabbix agent to load the module :

```
[TEST zbx_vmbix]# zabbix_agentd -t vmbix[version]
2.3.0.59
[TEST zbx_vmbix]# zabbix_agentd -t vmbix[vm.guest.os,VM01]
vmbix[vm.guest.os,VM01]                  [s|Red Hat Enterprise Linux 6 (64 bits)]
```

Or if VmBix is configured to use the UUID to reference the objects (useuuid parameter) :

```
[TEST zbx_vmbix]# zabbix_agentd -t "vmbix[vm.discovery,*]"
{
  "data": [
    {
      "{#VIRTUALMACHINE}": "VM01",
      "{#UUID}": "4214811c-1bab-f0fb-363b-9698a2dc607c"
    },
    {
      "{#VIRTUALMACHINE}": "VM01",
      "{#UUID}": "4214c939-18f1-2cd5-928a-67d83bc2f503"
    }
  ]
}
[TEST zbx_vmbix]# zabbix_agentd -t vmbix[vm.guest.os,4214811c-1bab-f0fb-363b-9698a2dc607c]
vmbix[vm.guest.os,VM01]                  [s|Red Hat Enterprise Linux 6 (64 bits)]
```

## Zabbix Templates
Sample templates using this loadable module are provided in the [VmBix repository](https://github.com/dav3860/vmbix/tree/master/zabbix).
