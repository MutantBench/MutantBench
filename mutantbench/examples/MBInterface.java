import py4j.GatewayServer;

public class MBInterface {

    public static String[] MBGetMutantLocations(String stringMutantLocations) {
        return stringMutantLocations.split(",");
    }

    public static int MBDetectMutants(String stringMutantLocations) {
        String [] mutantLocations = MBInterface.MBGetMutantLocations(stringMutantLocations);

        return 0;
    }


    public static void main(String[] args) {
        MBInterface app = new MBInterface();
        GatewayServer server = new GatewayServer(app);
        server.start();
    }
}
