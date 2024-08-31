# 🛠️ `fdisk` Utility

The `fdisk` utility reads and displays information about partitions on a specified disk device. This tool prints detailed partition information including boot status, start and end sectors, total sectors, size, and partition type.

## 📝 Features

- 📜 Displays primary and extended partitions.
- 🚀 Indicates if a partition is bootable.
- 📏 Shows partition sizes in MB or GB.
- 🛠️ Provides human-readable partition type descriptions.

## 📦 Prerequisites

- C compiler (e.g., `gcc`)
- Access to the disk device (e.g., `/dev/sda`)
- Root permissions to access disk information

## ⚙️ Compilation

To compile the `fdisk` utility, run:

```sh
gcc -o fdisk fdisk.c
```
## 🚀 Usage

To use the `fdisk` utility, run:

```sh
sudo ./fdisk /dev/sdX
