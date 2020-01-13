FROM python:3.7
ENV PYTHONUNBUFFERED 1
RUN mkdir /heartbeat
WORKDIR /heartbeat
ADD . /heartbeat/
RUN pip3 install .
RUN rm -rf /heartbeat
RUN useradd -ms /bin/bash heartbeat
USER heartbeat
WORKDIR /home/heartbeat
ENTRYPOINT ["heartbeat.py"]
