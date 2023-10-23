import json


def solve(filename):
    
    with open(filename, 'rb') as f:

        answer = {}

        b = f.read()

        edax_header = int.from_bytes(b[0:4], byteorder = "little")
        eval_header = int.from_bytes(b[4:8], byteorder = "little")

        version = int.from_bytes(b[8:12], byteorder = "little")
        release = int.from_bytes(b[12:16], byteorder = "little")
        build = int.from_bytes(b[16:20], byteorder = "little")
        date = int.from_bytes(b[20:28], byteorder = "little")

        print(f"edax_header = {edax_header}")
        print(f"eval_header = {eval_header}")
        print(f"version = {version}")
        print(f"release = {release}")
        print(f"build = {build}")
        print(f"date = {date}")

        answer["edax_header"] = str(edax_header)
        answer["eval_header"] = str(eval_header)
        answer["version"] = str(version)
        answer["release"] = str(release)
        answer["build"] = str(build)
        answer["date"] = str(date)
        answer["weight"] = {}

        for i in range(61):
            print(f"processing: {i} / 60")
            arr = []
            for j in range(114364):
                index = 28 + i * 114364 * 2 + j * 2
                n = int.from_bytes(b[index:index+2], byteorder="little", signed=True)
                arr.append(n)
            answer["weight"][str(i)] = arr

        ja = json.dumps(answer, sort_keys=False, indent=0)
        assert answer == json.loads(ja)

        return ja


if __name__ == "__main__":
    
    j = solve("data/eval.dat")
    with open("edax_eval_weight.json", "w") as f:
        f.write(j)