# P(ath) K(eeper) Command Tool - Multi-functional Command Line Tool  
## Project Overview  

<div style="background-color:#f9f9f9; border-left: 4px solid #007acc; padding: 10px; margin: 10px 0;">
  <strong>Tips:</strong>This branch would need extra packages(like Qt) to complete internationalization.
  If you do not want to build with Qt indeed, you can switch to main branch.
</div>


PathKeeper is a command-line tool designed for: 

- Recording frequently used commands
- Quickly switching and executing commands
- Supporting terminal type configuration

## Features  

- **Command Recording**: Add, view, and execute command history 
- **Multi-level Categorization**: Supports command categorization (e.g., 1.2 represents the second command under the first category)  
- **Configuration Management**: Configurable default execution terminal 

## Usage Tips  

1. **Quick Execution**: Use the `-p` parameter to quickly execute the most recently used command  
2. **Categorization Management**: Properly categorize commands when adding them for easier retrieval later  

## Example Workflow  

```bash  
# Execute the default command  
pk  

# Add development-related commands  
pk -a  

# View all records  
pk -s  

# Execute the first command  
pk -e 1  
```  

## Notes  

1. Command records are saved in a local file (`~/.pk.json`). Remember to back up important records  
2. When using the `-e` parameter, ensure the index is valid  

## Installation Instructions  

```bash  
# Install dependencies
# Ubuntu/Debian:
sudo apt update
sudo apt install cmake pkg-config libjsoncpp-dev build-essential

# CentOS/RHEL
sudo yum install cmake pkgconfig jsoncpp-devel gcc-c++

# Fedora
sudo dnf install cmake pkgconfig jsoncpp-devel gcc-c++

# And install Qt develop environment
# Ubuntu/Debian:
sudo apt install qt5-qmake qt5-default qt5-qmlviewer qt5-quickcontrols2-dev


# Compile the project  
mkdir build && cd build  
cmake ..  
make  

# Install to the system (requires sudo privileges)  
sudo make install  
```  

## Uninstallation Method  

```bash  
# Run in the build directory  
sudo make uninstall  
```  

Alternatively, uninstall manually:  

```bash  
# Manually delete installed files  
sudo rm /usr/local/bin/pk  
```  

## Contribution Guidelines  

Contributions and improvement suggestions are welcome! Please follow open-source community norms and submit PRs or Issues.  

## Open Source License  

This project adheres to an open-source license. For details, please refer to the `LICENSE` file in the project root directory.  

For further details about the code, you can review the source code of the corresponding modules.
