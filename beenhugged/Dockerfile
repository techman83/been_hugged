FROM python:3.7
ENV PYTHONUNBUFFERED 1
RUN apt-get update && apt-get install -y jq
RUN mkdir /beenhugged
WORKDIR /beenhugged
ADD . /beenhugged/
RUN pip3 install .
run pip3 install awscli
RUN rm -rf /beenhugged
RUN useradd -ms /bin/bash beenhugged
USER beenhugged
WORKDIR /home/newuser
COPY run.sh /bin/run.sh
CMD ["/bin/run.sh"]
