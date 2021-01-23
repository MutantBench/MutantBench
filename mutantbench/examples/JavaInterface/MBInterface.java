import java.io.File;
import java.io.IOException;
import py4j.GatewayServer;
import java.io.FileWriter;


public class MBInterface {

    public static String MBDetectMutants(String mutantLocation) {
        try {
            File myObj = new File("output.txt");
            if (myObj.createNewFile()) {
                System.out.println("File created: " + myObj.getName());
            } else {
                System.out.println("File already exists.");
            }
        } catch (IOException e) {
            System.out.println("An error occurred.");
            e.printStackTrace();
        }
        return "examples/JavaInterface/output.txt";
    }

    public static void main(String[] args) {
        MBInterface app = new MBInterface();
        GatewayServer server = new GatewayServer(app);
        server.start();
        System.out.println("Gateway Server Started");
    }
}
