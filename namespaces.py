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
        result.check_returncode()
    except subprocess.CalledProcessError as e:
        print("\033[0;31m", "ERROR: ", e, "\033[0m")
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

def add_ip(ip:str, ns:int, cidr:int=24, veth:int=-1):
    run_with_ns(ns, f"ip addr add {ip}/{cidr} dev veth_urouter{ns if veth==-1 else veth}")
    run_with_ns(ns, f"ip link set veth_urouter{ns if veth==-1 else veth} up")

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
    add_ip("192.168.3.1",1)
    print(run_with_ns(1, "ip a").stdout.decode())

    print("--- NS 2 ---")
    add_ip("192.168.3.2",2)
    print(run_with_ns(2, "ip a").stdout.decode())

def cleanup_topo_2_links():
    delete_namespace(1)
    delete_namespace(2)

def create_topo_3_links():
    """
    192.168.3.1                   192.168.3.4  
    [NS 1] <-----> [SWITCH] <-----> [NS 2]
    """
    create_namespace(1); create_namespace(2); create_namespace(3);
    print()
    create_veth(1,2); create_veth(3,4)
    print()
    set_veth(1,1)
    print()
    set_veth(2,2)
    print()
    set_veth(3,2)
    print()
    set_veth(4,3)

    print("--- NS 1 ---")
    add_ip("192.168.3.1",1)
    #print(run_with_ns(1, "ip a").stdout.decode())

    print("--- SWITCH ---")
    run_with_ns(2,"ip link set veth_urouter2 up")
    run_with_ns(2,"ip link set veth_urouter3 up")

    print("--- NS 2 ---")
    add_ip("192.168.3.4",3, veth=4)
    #print(run_with_ns(3, "ip a").stdout.decode())

def cleanup_topo_3_links():
    delete_namespace(1); delete_namespace(2); delete_namespace(3)

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
        create_topo_3_links()
    elif sys.argv[1] == 'remove':
        cleanup_topo_3_links()
    

    