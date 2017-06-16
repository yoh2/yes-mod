About yes-mod
=============
Yes-mod is a linux kernel module that implements a character device file
providing fixed string repatedly like `yes` command. The device file is
named `/dev/yes`.

Install
=======

 1. Build the kernel module.

    ```
    $ make
    ```

 2. Load the built module.

    ```
    $ sudo insmod yes.ko
    ```


Usage
=====

Reading /dev/yes provides repated mesage. The default string is `y`

```
$ head /dev/yes
y
y
y
y
y
y
y
y
y
y
```



Writing to /dev/yes changes the read string. NL will be appended if the last
character is not NL.

```
$ echo no > /dev/yes
$ head /dev/yes
no
no
no
no
no
no
no
no
no
no
```

Supplement
==========

To install the module to the system
------------------------------------------

You can install the module to the system and load from there.

```
$ sudo make install
$ sudo depmod
$ modprobe yes
```

I cannot open /dev/yes due to permission error
-----------------------------------------------

Depending to the system settings, /dev/yes cannot be opend except by root
user. To grant permission for other user to open the device file, you have
to create appropriate udev rules. There is a sample udev configuration file
`99-yes.rules` in the source directory. Putting that file into /etc/udev/rules.d
grant permission for all users to read/write /dev/yes from next module loading.
