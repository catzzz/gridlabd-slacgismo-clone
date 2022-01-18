FROM centos:8

RUN yum install git -y

RUN yum -q update -y ; 
RUN yum -q clean all
RUN yum -q groupinstall "Development Tools" -y
RUN yum -q install cmake -y 
RUN yum -q install ncurses-devel -y
RUN yum -q install epel-release -y
RUN yum -q install curl-devel -y
RUN yum -q install which 

RUN yum -q install svn -y
# RUN dnf install libnsl.i686
RUN dnf install libnsl -y


WORKDIR /usr/local/src


RUN	yum install gcc openssl-devel bzip2-devel libffi-devel zlib-devel  xz-devel  -y
RUN	curl https://www.python.org/ftp/python/3.9.6/Python-3.9.6.tgz | tar xz
WORKDIR /usr/local/src/Python-3.9.6
RUN	./configure --prefix=/usr/local --enable-optimizations --with-system-ffi --with-computed-gotos --enable-loadable-sqlite-extensions CFLAGS="-fPIC"
RUN	make -j 6
RUN	make altinstall
	
RUN	ln -sf /usr/local/bin/python3.9 /usr/local/bin/python3
RUN	ln -sf /usr/local/bin/python3.9-config /usr/local/bin/python3-config
RUN	ln -sf /usr/local/bin/pydoc3.9 /usr/local/bin/pydoc
RUN	ln -sf /usr/local/bin/idle3.9 /usr/local/bin/idle
RUN command	ln -sf /usr/local/bin/pip3.9 /usr/local/bin/pip3
	# install python packages
RUN	/usr/local/bin/python3 -m pip install --upgrade pip
RUN	/usr/local/bin/python3 -m pip install matplotlib Pillow pandas numpy networkx pytz pysolar PyGithub scikit-learn xlrd 
RUN	/usr/local/bin/python3 -m pip install IPython censusdata 


# # to install gdal 
#install jasper
# WORKDIR /usr/local/src
# RUN curl http://download.osgeo.org/gdal/jasper-1.900.1.uuid.tar.gz | tar xz
# # RUN tar xvf jasper-1.900.1.uuid.tar.gz

# WORKDIR /usr/local/src/jasper-1.900.1.uuid
# # # arm 64
# RUN ./configure --build=aarch64-unknown-linux-gnu 
# # #x86
# # #RUN ./configure --build=x86_64-unknown-linux-gnu

# RUN make -j6
# RUN make install


# # 1. proj6
# RUN yum install  proj-devel -y

# # install gdal
# WORKDIR /usr/local/src
# RUN curl http://download.osgeo.org/gdal/3.4.0/gdal-3.4.0.tar.gz | tar xz
# WORKDIR /usr/local/src/gdal-3.4.0
# RUN ./configure --with-python
# RUN make -j6
# RUN make install
# # install fiona and geopandas
# RUN /usr/local/bin/python3 -m pip install gdal==2.2.3

# WORKDIR "/usr/local/src/gdal-3.4.0"
# RUN ./configure --with-python
# RUN make -j6
# RUN make install

# using gdal package
# RUN yum install epel-release -y
RUN yum install gdal-devel 
 # path from `gdal-config --cflags`
RUN  export CPLUS_INCLUDE_PATH=/usr/include/gdal 
RUN export C_INCLUDE_PATH=/usr/include/gdal
RUN /usr/local/bin/python3 -m pip install GDAL==$(gdal-config --version | awk -F'[.]' '{print $1"."$2}')

# using pygdal package

RUN yum install gdal-devel
RUN /usr/local/bin/python3 -m pip install pygdal="`gdal-config --version`.*"
RUN /usr/local/bin/python3 -m pip install geopandas


#install gridlabd
WORKDIR /usr/local/src


RUN	/usr/local/bin/python3 -m pip install geopandas

RUN git clone -b develop https://github.com/slacgismo/gridlabd.git

WORKDIR /usr/local/src/gridlabd
COPY ./requirements.txt ./requirements.txt
COPY ./gldcore/geodata/geodata_utility.py ./gldcore/geodata/geodata_utility.py 
COPY ./gldcore/geodata/requirements.txt ./gldcore/geodata/requirements.txt
COPY ./gldcore/scripts/requirements.txt ./gldcore/scripts/requirements.txt
RUN autoreconf -isf && ./configure
RUN make -j6 system