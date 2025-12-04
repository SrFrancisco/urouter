import subprocess

PRINT_COMMANDS=True
DRY_RUN=False
assert (not DRY_RUN) or PRINT_COMMANDS


# ==============================================================================
# Network commands
# ==============================================================================

def run(arg:str):
    if PRINT_COMMANDS: print(f"==> {arg}")
    if not DRY_RUN:
        return _run(arg.split(" "))

def _run(arg:list[str]):
    try:
        result = subprocess.run(arg, stdout=subprocess.PIPE)
    except subprocess.CalledProcessError as e:
        print("ERROR: ", e)
        exit(1)
    return result

def get_namespaces():
    return run("ip netns list").stdout.decode()

def create_namespace(ns:int) -> int:
    run(f"ip netns add urouter{ns}")

def delete_namespace(ns:int):
    run(f"ip netns delete urouter{ns}")

def create_veth(iface1:int, iface2: int):
    run(f"ip link add veth_urouter{iface1} type veth peer name veth_urouter{iface2}")

def set_veth(iface: int, ns:int):
    run(f"ip link set veth_urouter{iface} netns urouter{ns}")
    activate_veth(ns, iface)

def remove_veth(iface: int):
    run(f"ip link delete veth_urouter{iface}")

def run_with_ns(ns:int, command:str):
    return run(f"ip netns exec urouter{ns} {command}")

def activate_veth(ns:int, num:int):
    run_with_ns(ns,f"ip link set veth_urouter{num} up")
    run_with_ns(ns,f"ip link set lo up")

# ==============================================================================
# TOPO
# ==============================================================================
def create_topo_2_links():
    """
        if1       if2
    [NS 1] <------> [NS 2]
    """

    create_namespace(1)
    create_namespace(2)
    create_veth(1,2)
    set_veth(1,1)
    set_veth(2,2)

    print("--- NS 1 ---")
    print(run_with_ns(1, "ip a").stdout.decode())

    print("--- NS 2 ---")
    print(run_with_ns(2, "ip a").stdout.decode())

def cleanup_topo_2_links():
    delete_namespace(1)
    delete_namespace(2)


# ==============================================================================
# ARGS
# ==============================================================================
if __name__=='__main__':
    import sys

    def usage():
        print("USAGE: namespaces.py command")
        print("Commands:")
        print("\tcreate\t\tcreate topo")
        print("\tremove\t\tremove topo")

    if len(sys.argv) != 2:
        usage()
        exit(1)

    if sys.argv[1] == 'create':
        create_topo_2_links()
    elif sys.argv[1] == 'remove':
        cleanup_topo_2_links()
    

    