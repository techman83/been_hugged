FROM python:3.9
ENV PYTHONUNBUFFERED 1
RUN mkdir /beenhugged
WORKDIR /beenhugged
ADD . /beenhugged/
RUN pip3 install .
RUN rm -rf /beenhugged
RUN useradd -ms /bin/bash beenhugged
USER beenhugged
WORKDIR /home/beenhugged
