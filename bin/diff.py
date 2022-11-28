#!/usr/bin/env python3

import argparse
import os
import cv2
import numpy as np
import colors


def load_mat(path):
    """
    Load an OpenCV matrix from the given path.
    """
    storage = cv2.FileStorage(path, cv2.FILE_STORAGE_READ)
    mat = storage.getFirstTopLevelNode().mat()
    return mat


class MatList:
    """
    Read from the lists.txt the list of the matrices written to a file
    """
    # The possible actions for each line of the file
    ENTER_SCOPE = 0
    EXIT_SCOPE = 1
    SAVE_MAT = 2

    def __init__(self, lists_path):
        """
        Constructor.
        """
        # Every action of the flow
        self.flow = []
        # Iterate the line as the flow order
        with open(lists_path, 'r') as file:
            # The first line is the file extension
            self.ext = file.readline().strip()
            # The following lines are each action in order (the execution flow)
            # Build the execution flow (self.flow)
            for line in file:
                # Remove begin/trailing whitespaces
                line = line.strip()
                if len(line) > 0:
                    if line[0] == '-':
                        # Exit a scope
                        self.flow.append({'type': self.EXIT_SCOPE})
                    elif line[0] == '+':
                        # Enter a scope, with a given name after the '+'
                        scope_name = line[1:].strip()
                        self.flow.append({'type': self.ENTER_SCOPE, 'name': scope_name})
                    else:
                        # Otherwise, this is a matrix save
                        # The line contains the name of the matrix
                        mat_name = line.strip()
                        self.flow.append({'type': self.SAVE_MAT, 'name': mat_name})

    def compare(self, tmp_dir1, tmp_dir2):
        """
        Compare two output directories.
        Base the files to compare on the lists file given in the constructor.
        Base the rendering order on the lists file given in the constructor.
        """
        # Stack of scopes for printing a tree
        scopes = []
        # Iterate all actions, in order
        for action in self.flow:
            # Get the type of the action
            # Do a different thing depending of the action type
            t = action['type']
            if t == self.ENTER_SCOPE:
                # Enter a scope
                scope_name = action['name']
                # Pretty print it
                print(colors.white("    " * len(scopes) + scope_name + "/"))
                # Push the scope
                scopes.append(scope_name)
            elif t == self.EXIT_SCOPE:
                # Exit a scope
                scopes.pop()
            elif t == self.SAVE_MAT:
                # Here the real work, we save a matrix.
                # Pretty print it
                # Get the name of the matrix
                mat_name = str(action['name'])

                # Get the path from the matrix name, scopes, and file extension
                # Both folder should have the same file extension than the one given in constructor
                # Attention to not path-join the extension, its part of the file name
                path1 = os.path.join(tmp_dir1, *scopes, mat_name + self.ext)
                path2 = os.path.join(tmp_dir2, *scopes, mat_name + self.ext)

                # Check if both files exists
                if os.path.isfile(path1) and os.path.isfile(path2):
                    # If so, load the matrices and compare them
                    try:
                        # Flatten both matrices into vectors
                        m1 = load_mat(path1).flatten()
                        m2 = load_mat(path2).flatten()
                        # Get the absolute difference
                        diff = np.abs(m2 - m1)
                        # Compute the L1 distance
                        d = np.linalg.norm(diff, ord=1)
                        # Compute more math. info.
                        min = np.min(diff)
                        max = np.max(diff)
                        avg = np.average(diff)
                        median = np.median(diff)
                        std = np.std(diff)
                        # Get the padding depending on the count of scopes
                        padding = "    " * len(scopes)
                        # Initial message when matrices are the same
                        # Data is appened to it if the matrices are different
                        text = padding + f'{mat_name}: d={d}'
                        # Are the matrices almost the same?
                        if d < 0.001:
                            # The matrices are almost the same, we can print it in green
                            print(colors.green(text))
                        else:
                            # The matrices are different, we print more information
                            # We also save the difference in files
                            text += f', min={min}, max={max}, avg={avg}, median={median}, std={std}'
                            # Different color if the different is big or relatively smaller
                            if d < 10.0:
                                print(colors.yellow(text))
                            else:
                                print(colors.red(text))
                            # In this case we save the difference
                            # In the same file format as the one given in the constructor from the lists file
                            # Create intermediates directories automatically
                            diff_dir = os.path.join(tmp_dir1, "diff", *scopes)
                            os.makedirs(diff_dir, exist_ok=True)
                            diff_file = os.path.join(diff_dir, mat_name + self.ext),
                            storage = cv2.FileStorage(diff_file, cv2.FILE_STORAGE_WRITE)
                            # Write the file, we don't care about the node name
                            storage.write("root", diff)
                    except Exception as e:
                        # If there is an error,
                        # I am almost sure this is because the dimension mismatch,
                        # Which can happen frequently if the executable versions or input parameters are different.
                        print(f'Error when comparing "{os.path.join(*scopes, mat_name + self.ext)}": "{e}".')
                else:
                    # Log on we can't find the files, maybe its a mistake and the user want to know...
                    print(f'Cannot find file matrices: "{path1}" or "{path2}"')
            else:
                raise Exception("Unknown action type: " + str(type))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='diff',
        description='Compare two output of the regressions tests.')

    parser.add_argument('tmp_dir1', help='Temporary directory of the first version')
    parser.add_argument('tmp_dir2', help='Temporary directory of the second version')

    args = parser.parse_args()

    # Use the lists of the first temporary directory
    # It doesn't matter as the matrix should appears in both sides
    # But it can change the visual output order if the algorithms flows differ
    info = MatList(os.path.join(args.tmp_dir1, "lists.txt"))
    info.compare(args.tmp_dir1, args.tmp_dir2)
