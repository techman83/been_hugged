FROM vimagick/mosquitto
RUN apk -v --update add \
	bash \
        python \
        py-pip \
        groff \
        less \
	jq \
        mailcap && rm /var/cache/apk/* 
# Install aws-cli
RUN pip install --upgrade awscli && apk -v --purge del py-pip
COPY mosquitto.conf /etc/mosquitto/.
COPY run.sh /bin/run.sh
ENTRYPOINT []
CMD ["/bin/bash", "-c", "/bin/run.sh"]
