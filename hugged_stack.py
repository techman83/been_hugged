import os

from troposphere import Base64, GetAtt
from troposphere import Join, Sub
from troposphere import Ref, Tags, Template
from troposphere.cloudformation import Init, InitFile, InitFiles, \
    InitConfig, InitService, Metadata
from troposphere.iam import Role, Policy
from troposphere.iam import InstanceProfile
from troposphere.route53 import RecordSetType
from troposphere.ecs import Cluster
from troposphere.ec2 import Instance, CreditSpecification


ZONE_ID = os.environ.get('HUGGED_ZONEID', False)
FQDN = 'hugged.techieman.net'
EMAIL = os.environ.get('HUGGED_EMAIL', False)
SECURITY_GROUP = 'sg-08f2b0b8afa5fa12f'

t = Template()

t.add_version("2010-09-09")

t.add_description("Been Hugged Infrastructure")

hugged_role = t.add_resource(Role(
    "HuggedRole",
    AssumeRolePolicyDocument={
        "Version": "2012-10-17",
        "Statement": [
            {
                "Effect": "Allow",
                "Principal": {
                    "Service": [
                        "ec2.amazonaws.com"
                    ]
                },
                "Action": [
                    "sts:AssumeRole"
                ]
            }
        ]
    },
    ManagedPolicyArns=[
        "arn:aws:iam::aws:policy/service-role/AmazonEC2ContainerServiceforEC2Role",
    ],
    Policies=[
        Policy(
            PolicyName="Certbot",
            PolicyDocument={
                "Version": "2012-10-17",
                "Statement": [
                    {
                        "Effect": "Allow",
                        "Action": [
                            "route53:ListHostedZones",
                            "route53:GetChange"
                        ],
                        "Resource": [
                            "*"
                        ]
                    },
                    {
                        "Effect": "Allow",
                        "Action": [
                            "route53:ChangeResourceRecordSets"
                        ],
                        "Resource": [
                            "arn:aws:route53:::hostedzone/{}".format(
                                ZONE_ID
                            ),
                        ]
                    }
                ],
            }
        ),
    ]
))


hugged_profile = t.add_resource(InstanceProfile(
    "HuggedProfile",
    Roles=[Ref(hugged_role)],
))

hugged_dns = t.add_resource(RecordSetType(
    "HuggedDnsRecord",
    HostedZoneId=ZONE_ID,
    Name=FQDN,
    Type="A",
    TTL="180",
    ResourceRecords=[GetAtt("EcsInstance", "PublicIp")],
))

hugged_ecs_role = t.add_resource(Role(
    "HuggedEcsRole",
    AssumeRolePolicyDocument={
        "Version": "2012-10-17",
        "Statement": [
            {
                "Effect": "Allow",
                "Principal": {
                    "Service": "ecs-tasks.amazonaws.com"
                },
                "Action": "sts:AssumeRole"
            }
        ]
    },
    Policies=[
        Policy(
            PolicyName="AllowParameterAccess",
            PolicyDocument={
                "Version": "2012-10-17",
                "Statement": [
                    {
                        "Effect": "Allow",
                        "Action": [
                            "ssm:DescribeParameters"
                        ],
                        "Resource": "*"
                    },
                    {
                        "Effect": "Allow",
                        "Action": [
                            "ssm:GetParameters"
                        ],
                        "Resource": "arn:aws:ssm:${AWS::Region}:${AWS::AccountId}:parameter/hugged/*",
                    }
                ]
            }
        )
    ]
))


hugged_cluster = t.add_resource(Cluster(
    "HuggedCluster",
    ClusterName="AllTheHugs",
))

cfn_hup = InitFile(
    content=Sub(
        "[main]\nstack=${AWS::StackId}\nregion=${AWS::Region}\n"
    ),
    mode='000400',
    owner='root',
    group='root'
)
reloader = InitFile(
    content=Sub("""
[cfn-auto-reloader-hook]
triggers=post.add, post.update
path=Resources.NetKANCompute.Metadata.AWS::CloudFormation::Init
action=/opt/aws/bin/cfn-init -s ${AWS::StackId} -r NetKANCompute --region ${AWS::Region}
runas=root
""")
)
docker = InitFile(
    content="""
{
    "log-driver": "json-file",
    "log-opts": {
        "max-size": "20m",
        "max-file": "3"
    }
}
""")
cfn_service = InitService(
    enabled=True,
    ensureRunning=True,
    files=[
        '/etc/cfn/cfn-hup.conf',
        '/etc/cfn/hooks.d/cfn-auto-reloader.conf',
    ]
)
docker_service = InitService(
    enabled=True,
    ensureRunning=True,
    files=['/etc/docker/daemon.json']
)

hugged_instance = t.add_resource(Instance(
    "HuggedInstance",
    Metadata=Init({
        "config": InitConfig(
            files=InitFiles({
                '/etc/cfn/cfn-hup.conf': cfn_hup,
                '/etc/cfn/hooks.d/cfn-auto-reloader.conf': reloader,
                '/etc/docker/daemon.json': docker,
            }),
        ),
        'services': {
            'sysvinit': {
                'cfn': cfn_service,
                'docker': docker_service,
            }
        },
        "commands": {
            "add_instance_to_cluster": {
                "command": "echo ECS_CLUSTER=$ECSCluster > /etc/ecs/ecs.config",
                "env": {"ECSCluster": Ref(hugged_cluster)}
            }
        }
    }),
    ImageId='ami-0adc350d7c7a2259f',
    KeyName='default',
    Tags=Tags(
        Name=Join("", [Ref("AWS::StackName")]),
        Application=Ref("AWS::StackId"),
    ),
    InstanceType='t3.nano',
    IamInstanceProfile=Ref(hugged_profile),
    SecurityGroupIds=[Ref(SECURITY_GROUP)],
    CreditSpecification=CreditSpecification(CPUCredits='standard'),
    UserData=Base64(Join(
        "",
        [
            "#!/bin/bash -xe\n",
            "yum install -y aws-cfn-bootstrap\n",
            "# Install the files and packages from the metadata\n",
            "/opt/aws/bin/cfn-init -v ",
            "         --stack ", Ref("AWS::StackName"),
            "         --resource EcsInstance ",
            "         --region ",
            Ref("AWS::Region"),
            "\n",
            "# Get our certificates\n",
            "docker run -v /opt/letsencrypt:/etc/letsencrypt/ certbot/dns-route53 certonly -n --agree-tos --email ",
            Ref(EMAIL),
            " --dns-route53 -d ",
            Ref(FQDN),
            "\n",
            "(cd /opt && mkdir certs && cp -L letsencrypt/live/",
            Ref(FQDN),
            "/*.pem certs/.)\n",
            "# Should do something better than this\n",
            "chmod 0644 /opt/certs/privkey.pem\n",
            "# Start up the cfn-hup daemon to listen for changes to the metadata\n",
            "/opt/aws/bin/cfn-hup || error_exit 'Failed to start cfn-hup'\n",
            "# Signal the status from cfn-init\n", "/opt/aws/bin/cfn-signal -e $? ",
            "         --stack ", Ref("AWS::StackName"),
            "         --resource EcsInstance ",
            "         --region ",
            Ref("AWS::Region"), "\n"
        ]
    )),
))

print(t.to_yaml())
