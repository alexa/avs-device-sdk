# Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
#
#     http://aws.amazon.com/apache2.0/
#
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.

from argparse import ArgumentParser
from subprocess import check_call, check_output
from os import devnull, path
import string

# Used devnull to redirect output.
FNULL = open(devnull, 'w')

# Android uses forward slash to separate folders.
SEPARATOR = '/'


def run_test(args):
    """Upload and run the test on the device."""
    lib_folder = string.join([args.device_path, 'lib'], SEPARATOR)
    test_folder = string.join([args.device_path, 'test'], SEPARATOR)
    input_folder = string.join([args.device_path, 'input'], SEPARATOR)
    test_path = string.join([test_folder, args.name], SEPARATOR)

    # Setup the device folders.
    check_call('adb shell mkdir -p {}'.format(test_folder).split(),
               stdout=FNULL)
    check_call('adb shell mkdir -p {}'.format(input_folder).split(),
               stdout=FNULL)
    check_call('adb push {} {}'.format(args.source, test_folder).split(),
               stdout=FNULL)

    # Upload necessary input files and setup arguments.
    inputs = process_inputs(input_folder, args.inputs)

    # Run test command on test folder and with installed libraries.
    cd_command = 'cd {};'.format(test_folder)
    export_command = 'export LD_LIBRARY_PATH={};'.format(lib_folder)
    test_command = '{} {};'.format(test_path, inputs)

    return check_output(['adb',
                         'shell',
                         cd_command,
                         export_command,
                         test_command,
                         'echo $?'])


def parse():
    """Parse arguments."""
    parser = ArgumentParser()
    parser.add_argument('-n',
                        '--name',
                        type=str,
                        help='The test name.')

    parser.add_argument('-s',
                        '--source',
                        type=str,
                        help='The test path on the host device.')

    parser.add_argument('-d',
                        '--device_path',
                        type=str,
                        help='The install folder path on the android device.')

    parser.add_argument('-i',
                        '--inputs',
                        nargs='*',
                        help='The inputs to be given to the test.')

    return parser.parse_args()


def process_inputs(inputs_folder, inputs):
    """Process the inputs passed to the test.

    If the input is an existing file or directory, upload it to the device and
    use the new location as argument.
    """
    ret = []
    for toProcess in inputs:
        if path.exists(toProcess):
            # Input is a file / directory. Upload it to the device.
            push_command = 'adb push {} {}'.format(toProcess, inputs_folder)
            check_call(push_command.split(), stdout=FNULL)
            basename = path.basename(toProcess)
            ret.append(string.join([inputs_folder, basename], SEPARATOR))
        else:
            ret.append(toProcess)
    return string.join(ret)


def parse_output(output):
    """Extract test return value and print the test output."""
    lines = output.splitlines()
    print(string.join(lines[0:-1], '\n'))
    return int(lines[-1])

if __name__ == '__main__':
    args = parse()
    output = run_test(args)
    ret_value = parse_output(output)
    exit(ret_value)
