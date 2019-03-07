1.2
* Added support for querying multiple instances of VMBix. An optional connection
  string can be added as the first parameter to specify the VMBix instance.
  - without connection string, the Zabbix item would be for example :
  ```
  vmbix[vmbix.ping]
  ```
  - with a connection string, the Zabbix item would then be for example :
  ```
  vmbix[tcp://127.0.0.1:14050,vmbix.ping]
  ```

1.1
* Removed support for Zabbix 2.4 and added Zabbix 3.4

1.0
* Added the support for Zabbix 3.2 loadable modules
