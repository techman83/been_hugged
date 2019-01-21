FROM python:3.7
ENV PYTHONUNBUFFERED 1
RUN apt-get update && apt-get install -y jq
RUN mkdir /heartbeat
WORKDIR /heartbeat
ADD . /heartbeat/
RUN pip3 install .
run pip3 install awscli
RUN rm -rf /heartbeat
RUN useradd -ms /bin/bash heartbeat
USER heartbeat
WORKDIR /home/heartbeat
COPY run.sh /bin/run.sh
CMD ["/bin/run.sh"]
