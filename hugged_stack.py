import os

from troposphere import Base64, GetAtt
from troposphere import Join, Sub
from troposphere import Ref, Tags, Template
from troposphere.cloudformation import Init, InitFile, InitFiles, \
    InitConfig, InitService, Metadata
from troposphere.iam import Role, Policy
from troposphere.iam import InstanceProfile
from troposphere.route53 import RecordSetType
from troposphere.ecs import Cluster, TaskDefinition, ContainerDefinition, \
    Secret, Environment, Volume, Host, MountPoint, PortMapping, Service, \
    DeploymentConfiguration, ContainerDependency
from troposphere.ec2 import Instance, CreditSpecification


ZONE_ID = os.environ.get('HUGGED_ZONEID', False)
FQDN = 'hugged.techieman.net'
EMAIL = os.environ.get('HUGGED_EMAIL', False)
SECURITY_GROUP = 'sg-08f2b0b8afa5fa12f'
TEST_MODE = '1'
TIMEZONE = 'Australia/Queensland'

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
    ResourceRecords=[GetAtt("HuggedInstance", "PublicIp")],
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
                        "Resource": Sub("arn:aws:ssm:${AWS::Region}:${AWS::AccountId}:parameter/hugged/*"),
                    }
                ]
            }
        ),
        # I've never needed this for ECR before -_-
        Policy(
            PolicyName="Ecr",
            PolicyDocument={
                "Version": "2012-10-17",
                "Statement": [
                    {
                        "Effect": "Allow",
                        "Action": [
                            "ecr:GetAuthorizationToken",
                            "logs:CreateLogStream",
                            "logs:PutLogEvents"
                        ],
                        "Resource": "*"
                    },
                    {
                        "Effect": "Allow",
                        "Action": [
                            "ecr:BatchCheckLayerAvailability",
                            "ecr:GetDownloadUrlForLayer",
                            "ecr:BatchGetImage"
                        ],
                        "Resource": "*",
                    }
                ]
            }
        ),
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
        }
    }),
    ImageId='ami-0adc350d7c7a2259f',
    KeyName='Seras',
    Tags=Tags(
        Name=Join("", [Ref("AWS::StackName")]),
        Application=Ref("AWS::StackId"),
    ),
    InstanceType='t3.nano',
    IamInstanceProfile=Ref(hugged_profile),
    SecurityGroupIds=[SECURITY_GROUP],
    CreditSpecification=CreditSpecification(CPUCredits='standard'),
    UserData=Base64(Join(
        "",
        [
            "#!/bin/bash -xe\n",
            "echo ECS_CLUSTER=AllTheHugs > /etc/ecs/ecs.config\n",
            "yum install -y aws-cfn-bootstrap\n",
            "# Install the files and packages from the metadata\n",
            "/opt/aws/bin/cfn-init -v ",
            "         --stack ", Ref("AWS::StackName"),
            "         --resource HuggedInstance ",
            "         --region ",
            Ref("AWS::Region"),
            "\n",
            "# Get our certificates\n",
            "docker run -v /opt/letsencrypt:/etc/letsencrypt/ certbot/dns-route53 certonly -n --agree-tos --email ",
            EMAIL,
            " --dns-route53 -d ",
            FQDN,
            "\n",
            "(cd /opt && mkdir certs && cp -L letsencrypt/live/",
            FQDN,
            "/*.pem certs/.)\n",
            "# Should do something better than this\n",
            "chmod 0644 /opt/certs/privkey.pem\n",
            "# Start up the cfn-hup daemon to listen for changes to the metadata\n",
            "/opt/aws/bin/cfn-hup || error_exit 'Failed to start cfn-hup'\n",
            "# Signal the status from cfn-init\n", "/opt/aws/bin/cfn-signal -e $? ",
            "         --stack ", Ref("AWS::StackName"),
            "         --resource HuggedInstance ",
            "         --region ",
            Ref("AWS::Region"), "\n"
        ]
    )),
))

containers = [
    {
        'name': 'hugged',
        'memory': '128',
        'image': 'beenhugged',
        'secrets': [
            'twitter_ckey', 'twitter_csecret', 'twitter_akey',
            'twitter_asecret', 'mqtt_user', 'mqtt_pass'
        ],
        'env': [
            ('TEST_MODE', TEST_MODE),
            ('TWITTER_HANDLE', 'techman83'),
            ('QUALITY_MODIFIER', '10'),
            ('TIMEZONE', TIMEZONE),
        ],
        'depends': ['mosquitto']
    },
    {
        'name': 'heartbeat',
        'memory': '128',
        'secrets': [
            'mqtt_user', 'mqtt_pass'
        ],
        'env': [
            ('INTERVAL', '5'),
        ],
        'depends': ['mosquitto']
    },
    {
        'name': 'mosquitto',
        'secrets': [
            'mqtt_user', 'mqtt_pass',
            'mqtt_badge_user', 'mqtt_badge_pass'
        ],
        'ports': ['8883', '1883'],
        'volumes': [
            ('/opt/certs/fullchain.pem', '/var/lib/mosquitto/fullchain.pem'),
            ('/opt/certs/privkey.pem', '/var/lib/mosquitto/privkey.pem'),
        ],
    },
]

task = TaskDefinition(
    'HuggingTask',
    ContainerDefinitions=[],
    Family=Sub('${AWS::StackName}${name}', name='Hugging'),
    ExecutionRoleArn=Ref(hugged_ecs_role),
    Volumes=[],
    DependsOn=[],
)

for container in containers:
    secrets = container.get('secrets', [])
    envs = container.get('env', [])
    entrypoint = container.get('entrypoint')
    command = container.get('command')
    volumes = container.get('volumes', [])
    ports = container.get('ports', [])
    depends = container.get('depends', [])
    definition = ContainerDefinition(
        Image=Sub(
            '${AWS::AccountId}.dkr.ecr.${AWS::Region}.amazonaws.com/been_hugged/${name}',
            name=container.get('image', container['name'])
        ),
        Memory=container.get('memory', '96'),
        Name=container['name'],
        Secrets=[
            Secret(
                Name=x.upper(),
                ValueFrom='/hugged/{}'.format(x)
            ) for x in secrets
        ],
        Environment=[
            Environment(
                Name=x[0].upper(), Value=x[1]
            ) for x in envs
        ],
        MountPoints=[],
        PortMappings=[],
        DependsOn=[],
        Links=[],
    )
    if entrypoint:
        entrypoint = entrypoint if isinstance(entrypoint, list) else [entrypoint]
        definition.EntryPoint = entrypoint
    if command:
        command = command if isinstance(command, list) else [command]
        definition.Command = command
    for volume in volumes:
        volume_name = '{}{}'.format(
            container['name'],
            ''.join([i for i in volume[0].capitalize() if i.isalpha()])
        )
        task.Volumes.append(
            Volume(
                Name=volume_name,
                Host=Host(
                    SourcePath=('{}'.format(volume[0]))
                )
            )
        )
        definition.MountPoints.append(
            MountPoint(
                ContainerPath=volume[1],
                SourceVolume=volume_name
            )
        )
    for port in ports:
        definition.PortMappings.append(
            PortMapping(
                ContainerPort=port,
                HostPort=port,
                Protocol='tcp',
            )
        )
    for depend in depends:
        definition.DependsOn.append(
            ContainerDependency(
                Condition='START',
                ContainerName=depend,
            )
        )
        definition.Links.append(depend)
    task.ContainerDefinitions.append(definition)
t.add_resource(task)

t.add_resource(Service(
    'HuggedService',
    Cluster=Ref(hugged_cluster),
    DesiredCount=1,
    TaskDefinition=Ref(task),
    # Allow for in place service redeployments
    DeploymentConfiguration=DeploymentConfiguration(
        MaximumPercent=100,
        MinimumHealthyPercent=0
    ),
    DependsOn=['HuggedCluster']
))


print(t.to_yaml())
