// This is mutant program.
// Author : ysma

package pamvotis.core;

import java.io.File;
import java.util.Vector;

import javax.xml.parsers.DocumentBuilderFactory;

import pamvotis.sources.RandomGenerator;
import pamvotis.sources.Source;

public class Simulator {

	public Simulator() {
		SpecParams.ReadParameters();
	}

	private int seed = 0;

	private long totalTime = 0;

	private long simTime = 0;

	private double currentTime = 0;

	private int nmbrOfNodes = 0;

	private int mixNodes = 0;

	private int rtsThr = 0;

	private char ctsToSelf = 'n';

	private char phyLayer = 's';

	private int cwMin;

	private float sifs;

	private float slot;

	private java.lang.String resultsPath = null;

	private java.lang.String outResults = null;

	private short progress = 0;

	private java.io.BufferedWriter out = null;

	private java.util.Random generator;

	private boolean transmissionPending = false;

	private int transTimeRemaining = 0;

	private boolean transmitWithRTS = false;

	private static final int INT_MAX = 999999999;

	private Vector<MobileNode> nodesList = new Vector<>();

	private int cwMinFact0 = 0;

	private int cwMinFact1 = 0;

	private int cwMinFact2 = 0;

	private int cwMinFact3 = 0;

	private int cwMaxFact0 = 0;

	private int cwMaxFact1 = 0;

	private int cwMaxFact2 = 0;

	private int cwMaxFact3 = 0;

	private int aifs0 = 0;

	private int aifs1 = 0;

	private int aifs2 = 0;

	private int aifs3 = 0;

	private void fightForSlot() {
		int distance = INT_MAX;
		int transRequests = 0;
		int transNlos = 0;
		int coverage = INT_MAX;
		for (int i = 0; i < nmbrOfNodes; i++) {
			if (((pamvotis.core.MobileNode) nodesList.elementAt(i)).nowTransmitting == false
					&& ((pamvotis.core.MobileNode) nodesList.elementAt(i)).havePktToSend == true
					&& ((pamvotis.core.MobileNode) nodesList.elementAt(i)).backoffCounter == 0) {
				((pamvotis.core.MobileNode) nodesList.elementAt(i)).requestTransmit = true;
			}
			transRequests = transRequests
					+ (((pamvotis.core.MobileNode) nodesList.elementAt(i)).requestTransmit ? 1
							: 0);
		}
		if (transmissionPending == true) {
			if (transmitWithRTS == true) {
				freeze();
			} else {
				for (int i = 0; i < nmbrOfNodes; i++) {
					distance = INT_MAX;
					for (int j = 0; j < nmbrOfNodes; j++) {
						if (((pamvotis.core.MobileNode) nodesList.elementAt(j)).nowTransmitting == true
								&& distance > ((pamvotis.core.MobileNode) nodesList
										.elementAt(i)).params.DistFrom(
										((pamvotis.core.MobileNode) nodesList
												.elementAt(j)).params.x,
										((pamvotis.core.MobileNode) nodesList
												.elementAt(j)).params.y)) {
							distance = ((pamvotis.core.MobileNode) nodesList
									.elementAt(i)).params.DistFrom(
									((pamvotis.core.MobileNode) nodesList
											.elementAt(j)).params.x,
									((pamvotis.core.MobileNode) nodesList
											.elementAt(j)).params.y);
						}
					}
					coverage = ((pamvotis.core.MobileNode) nodesList
							.elementAt(i)).params.coverage;
					if (distance > coverage
							&& ((pamvotis.core.MobileNode) nodesList
									.elementAt(i)).requestTransmit == true) {
						transNlos++;
					} else {
						if (((pamvotis.core.MobileNode) nodesList.elementAt(i)).backoffCounter > 0
								&& distance > coverage
								&& ((pamvotis.core.MobileNode) nodesList
										.elementAt(i)).nowTransmitting == false) {
							((pamvotis.core.MobileNode) nodesList.elementAt(i)).backoffCounter--;
						}
					}
				}
				if (transNlos > 0) {
					collision();
				} else {
					freeze();
				}
			}
		} else {
			if (transRequests == 0) {
				emptySlot();
			} else {
				if (transRequests == 1) {
					transmitWithRTS = successfulTransmission();
				} else {
					collision();
				}
				for (int i = 0; i < nmbrOfNodes; i++) {
					distance = INT_MAX;
					for (int j = 0; j < nmbrOfNodes; j++) {
						if (((pamvotis.core.MobileNode) nodesList.elementAt(j)).nowTransmitting == true
								&& distance > ((pamvotis.core.MobileNode) nodesList
										.elementAt(i)).params.DistFrom(
										((pamvotis.core.MobileNode) nodesList
												.elementAt(j)).params.x,
										((pamvotis.core.MobileNode) nodesList
												.elementAt(j)).params.y)) {
							distance = ((pamvotis.core.MobileNode) nodesList
									.elementAt(j)).params.DistFrom(
									((pamvotis.core.MobileNode) nodesList
											.elementAt(j)).params.x,
									((pamvotis.core.MobileNode) nodesList
											.elementAt(j)).params.y);
						}
					}
					coverage = ((pamvotis.core.MobileNode) nodesList
							.elementAt(i)).params.coverage;
					if (((pamvotis.core.MobileNode) nodesList.elementAt(i)).backoffCounter > 0
							&& distance > coverage
							&& ((pamvotis.core.MobileNode) nodesList
									.elementAt(i)).nowTransmitting == false) {
						((pamvotis.core.MobileNode) nodesList.elementAt(i)).backoffCounter--;
					}
				}
			}
		}
	}

	private void emptySlot() {
		for (int i = 0; i < nmbrOfNodes; i++) {
			if (((pamvotis.core.MobileNode) nodesList.elementAt(i)).backoffCounter > 0) {
				((pamvotis.core.MobileNode) nodesList.elementAt(i)).backoffCounter--;
			}
		}
		transmissionPending = false;
	}

	private boolean successfulTransmission() {
		int transNode = -1;
		boolean transWithRTS = true;
		float probOFDM = 0;
		char transType = 'O';
		int ACK = SpecParams.ACK;
		int RTS = SpecParams.RTS;
		int CTS = SpecParams.CTS;
		int MAC = SpecParams.MAC;
		float OFDM_PHY = SpecParams.OFDM_PHY;
		for (int i = 0; i < nmbrOfNodes; i++) {
			if (((pamvotis.core.MobileNode) nodesList.elementAt(i)).requestTransmit == true) {
				transNode = i;
				((pamvotis.core.MobileNode) nodesList.elementAt(i)).successfullyTransmitting = true;
				((pamvotis.core.MobileNode) nodesList.elementAt(i)).nowTransmitting = true;
				((pamvotis.core.MobileNode) nodesList.elementAt(i)).requestTransmit = false;
			}
		}
		probOFDM = (float) (nmbrOfNodes - mixNodes) / (float) nmbrOfNodes
				* (float) (nmbrOfNodes - mixNodes - 1) / (nmbrOfNodes - 1);
		float rand01 = generator.nextFloat();
		if (phyLayer == 'm') {
			if (rand01 < probOFDM) {
				transType = 'O';
			} else {
				transType = 'D';
			}
		} else {
			transType = 'U';
		}
		int payld = ((pamvotis.core.MobileNode) nodesList.elementAt(transNode)).pktLength;
		int rate = ((pamvotis.core.MobileNode) nodesList.elementAt(transNode)).params.rate;
		if (phyLayer == 'a' || phyLayer == 'g' || transType == 'O') {
			if (payld > rtsThr) {
				if (ctsToSelf == 'n') {
					transWithRTS = true;
					transTimeRemaining = (int) ((((pamvotis.core.MobileNode) nodesList
							.elementAt(transNode)).params.aifsd + 3 * sifs + 4 * OFDM_PHY) / slot)
							+ (RTS + CTS + MAC + payld + ACK
									+ padBits(rate, RTS) + padBits(rate, CTS)
									+ padBits(rate, MAC) + padBits(rate, payld) + padBits(
										rate, ACK)) / (int) (rate * slot);
				} else {
					transWithRTS = false;
					transTimeRemaining = (int) ((((pamvotis.core.MobileNode) nodesList
							.elementAt(transNode)).params.aifsd + 2 * sifs + 3 * OFDM_PHY) / slot)
							+ (CTS + MAC + payld + ACK + padBits(rate, CTS)
									+ padBits(rate, MAC) + padBits(rate, payld) + padBits(
										rate, ACK)) / (int) (rate * slot);
				}
			} else {
				transWithRTS = false;
				transTimeRemaining = (int) ((((pamvotis.core.MobileNode) nodesList
						.elementAt(transNode)).params.aifsd + sifs + 2 * OFDM_PHY) / slot)
						+ (MAC + payld + ACK + padBits(rate, MAC)
								+ padBits(rate, payld) + padBits(rate, ACK))
						/ (int) (rate * slot);
			}
		}
		if (phyLayer == 'b' || phyLayer == 's' || transType == 'D') {
			float phy = (float) SpecParams.SHORT_PHY;
			if (phyLayer == 's') {
				phy = (float) SpecParams.LONG_PHY;
			}
			if (payld > rtsThr) {
				if (ctsToSelf == 'n') {
					transWithRTS = true;
					transTimeRemaining = (int) ((((pamvotis.core.MobileNode) nodesList
							.elementAt(transNode)).params.aifsd + 3 * sifs + 4 * phy) / slot)
							+ (RTS + CTS + MAC + payld + ACK)
							/ (int) (rate * slot);
				} else {
					transWithRTS = false;
					transTimeRemaining = (int) ((((pamvotis.core.MobileNode) nodesList
							.elementAt(transNode)).params.aifsd + 2 * sifs + 3 * phy) / slot)
							+ (CTS + MAC + payld + ACK) / (int) (rate * slot);
				}
			} else {
				transWithRTS = false;
				transTimeRemaining = (int) ((((pamvotis.core.MobileNode) nodesList
						.elementAt(transNode)).params.aifsd + sifs + 2 * phy) / slot)
						+ (MAC + payld + ACK) / (int) (rate * slot);
			}
		}
		transmissionPending = true;
		freeze();
		return transWithRTS;
	}

	private void collision() {
		int maxPld = 0;
		int maxLsThr = 0;
		float maxTrans = 0;
		float maxLsTrans = 0;
		int maxNode = -1;
		int maxLsNode = -1;
		int RTS = SpecParams.RTS;
		int CTS = SpecParams.CTS;
		int MAC = SpecParams.MAC;
		int ACK = SpecParams.ACK;
		float OFDM_PHY = SpecParams.OFDM_PHY;
		int transTimeRemainingOld = 0;
		boolean los = true;
		int rate = 0;
		int payld = 0;
		int distance = -1;
		int coverage = INT_MAX;
		transTimeRemainingOld = transTimeRemaining;
		for (int i = 0; i < nmbrOfNodes; i++) {
			los = true;
			distance = INT_MAX;
			if (((pamvotis.core.MobileNode) nodesList.elementAt(i)).nowTransmitting == true) {
				continue;
			}
			for (int j = 0; j < nmbrOfNodes; j++) {
				if (((pamvotis.core.MobileNode) nodesList.elementAt(j)).nowTransmitting == true
						&& distance > ((pamvotis.core.MobileNode) nodesList
								.elementAt(i)).params.DistFrom(
								((pamvotis.core.MobileNode) nodesList
										.elementAt(j)).params.x,
								((pamvotis.core.MobileNode) nodesList
										.elementAt(j)).params.y)) {
					distance = ((pamvotis.core.MobileNode) nodesList
							.elementAt(i)).params
							.DistFrom(((pamvotis.core.MobileNode) nodesList
									.elementAt(j)).params.x,
									((pamvotis.core.MobileNode) nodesList
											.elementAt(j)).params.y);
				}
			}
			coverage = ((pamvotis.core.MobileNode) nodesList.elementAt(i)).params.coverage;
			if (distance > coverage || distance == INT_MAX) {
				los = false;
			} else {
				los = true;
			}
			if (((pamvotis.core.MobileNode) nodesList.elementAt(i)).requestTransmit == true
					&& los == false) {
				((pamvotis.core.MobileNode) nodesList.elementAt(i)).startTransmitting = true;
				if (((pamvotis.core.MobileNode) nodesList.elementAt(i)).pktLength >= maxPld) {
					maxPld = ((pamvotis.core.MobileNode) nodesList.elementAt(i)).pktLength;
				}
				if (((pamvotis.core.MobileNode) nodesList.elementAt(i)).pktLength <= rtsThr) {
					maxLsThr = ((pamvotis.core.MobileNode) nodesList
							.elementAt(i)).pktLength;
				}
				if ((float) ((pamvotis.core.MobileNode) nodesList.elementAt(i)).pktLength
						/ (float) ((pamvotis.core.MobileNode) nodesList
								.elementAt(i)).params.rate > maxLsTrans
						&& ((pamvotis.core.MobileNode) nodesList.elementAt(i)).pktLength <= rtsThr) {
					maxLsTrans = (float) ((pamvotis.core.MobileNode) nodesList
							.elementAt(i)).pktLength
							/ (float) ((pamvotis.core.MobileNode) nodesList
									.elementAt(i)).params.rate;
					maxLsNode = i;
				}
				if ((float) ((pamvotis.core.MobileNode) nodesList.elementAt(i)).pktLength
						/ (float) ((pamvotis.core.MobileNode) nodesList
								.elementAt(i)).params.rate > maxTrans) {
					maxTrans = (float) ((pamvotis.core.MobileNode) nodesList
							.elementAt(i)).pktLength
							/ (float) ((pamvotis.core.MobileNode) nodesList
									.elementAt(i)).params.rate;
					maxNode = i;
				}
				if (((pamvotis.core.MobileNode) nodesList.elementAt(i)).contWind < ((pamvotis.core.MobileNode) nodesList
						.elementAt(i)).params.cwMax) {
					((pamvotis.core.MobileNode) nodesList.elementAt(i)).contWind *= 2;
				} else {
					((pamvotis.core.MobileNode) nodesList.elementAt(i)).contWind = ((pamvotis.core.MobileNode) nodesList
							.elementAt(i)).params.cwMax;
				}
				((pamvotis.core.MobileNode) nodesList.elementAt(i)).backoffCounter = ((pamvotis.core.MobileNode) nodesList
						.elementAt(i))
						.InitBackoff(((pamvotis.core.MobileNode) nodesList
								.elementAt(i)).contWind);
				((pamvotis.core.MobileNode) nodesList.elementAt(i)).requestTransmit = false;
				((pamvotis.core.MobileNode) nodesList.elementAt(i)).collisions++;
			}
		}
		if (maxPld < rtsThr) {
			rate = ((pamvotis.core.MobileNode) nodesList.elementAt(maxNode)).params.rate;
			payld = ((pamvotis.core.MobileNode) nodesList.elementAt(maxNode)).pktLength;
		} else {
			if (maxLsThr != 0) {
				rate = ((pamvotis.core.MobileNode) nodesList
						.elementAt(maxLsNode)).params.rate;
				payld = ((pamvotis.core.MobileNode) nodesList
						.elementAt(maxLsNode)).pktLength;
			} else {
				rate = ((pamvotis.core.MobileNode) nodesList.elementAt(maxNode)).params.rate;
				payld = ((pamvotis.core.MobileNode) nodesList
						.elementAt(maxNode)).pktLength;
			}
		}
		if (phyLayer == 'a' || phyLayer == 'g') {
			if (maxLsThr != 0) {
				transTimeRemaining = (int) ((((pamvotis.core.MobileNode) nodesList
						.elementAt(maxLsNode)).params.aifsd + OFDM_PHY) / slot)
						+ (MAC + payld + padBits(rate, MAC) + padBits(rate,
								payld)) / (int) (rate * slot);
			} else {
				if (ctsToSelf == 'n') {
					transTimeRemaining = (int) ((((pamvotis.core.MobileNode) nodesList
							.elementAt(maxNode)).params.aifsd + 2 * OFDM_PHY + sifs) / slot)
							+ (RTS + ACK + padBits(6000000, RTS) + padBits(
									6000000, ACK)) / (int) (6000000 * slot);
				} else {
					transTimeRemaining = (int) ((((pamvotis.core.MobileNode) nodesList
							.elementAt(maxNode)).params.aifsd + OFDM_PHY) / slot)
							+ (CTS + padBits(6000000, CTS))
							/ (int) (6000000 * slot);
				}
			}
		} else {
			float phy = (float) SpecParams.SHORT_PHY;
			if (phyLayer == 's') {
				phy = (float) SpecParams.LONG_PHY;
			}
			if (maxLsThr != 0) {
				transTimeRemaining = (int) ((((pamvotis.core.MobileNode) nodesList
						.elementAt(maxLsNode)).params.aifsd + phy) / slot)
						+ (MAC + payld) / (int) (rate * slot);
			} else {
				if (ctsToSelf == 'n') {
					transTimeRemaining = (int) ((((pamvotis.core.MobileNode) nodesList
							.elementAt(maxNode)).params.aifsd + 2 * phy + sifs) / slot)
							+ (RTS + ACK) / (int) (1000000 * slot);
				} else {
					transTimeRemaining = (int) ((((pamvotis.core.MobileNode) nodesList
							.elementAt(maxNode)).params.aifsd + phy) / slot)
							+ CTS / (int) (1000000 * slot);
				}
			}
		}
		for (int i = 0; i < nmbrOfNodes; i++) {
			if (((pamvotis.core.MobileNode) nodesList.elementAt(i)).nowTransmitting == true) {
				if (((pamvotis.core.MobileNode) nodesList.elementAt(i)).contWind < ((pamvotis.core.MobileNode) nodesList
						.elementAt(i)).params.cwMax) {
					((pamvotis.core.MobileNode) nodesList.elementAt(i)).contWind *= 2;
				} else {
					((pamvotis.core.MobileNode) nodesList.elementAt(i)).contWind = ((pamvotis.core.MobileNode) nodesList
							.elementAt(i)).params.cwMax;
				}
				((pamvotis.core.MobileNode) nodesList.elementAt(i)).backoffCounter = ((pamvotis.core.MobileNode) nodesList
						.elementAt(i))
						.InitBackoff(((pamvotis.core.MobileNode) nodesList
								.elementAt(i)).contWind);
				((pamvotis.core.MobileNode) nodesList.elementAt(i)).requestTransmit = false;
				((pamvotis.core.MobileNode) nodesList.elementAt(i)).collisions++;
				((pamvotis.core.MobileNode) nodesList.elementAt(i)).successfullyTransmitting = false;
			}
		}
		for (int i = 0; i < nmbrOfNodes; i++) {
			if (((pamvotis.core.MobileNode) nodesList.elementAt(i)).startTransmitting == true) {
				((pamvotis.core.MobileNode) nodesList.elementAt(i)).startTransmitting = false;
				((pamvotis.core.MobileNode) nodesList.elementAt(i)).nowTransmitting = true;
			}
		}
		if (transTimeRemaining < transTimeRemainingOld) {
			transTimeRemaining = transTimeRemainingOld;
		}
		transmissionPending = true;
		freeze();
	}

	private void printStats() {
		float result = 0;
		float printTime = (float) (Math.round(currentTime * 10) / 10d);
		try {
			if (outResults.contains("tb")) {
				out = new java.io.BufferedWriter(new java.io.FileWriter(
						resultsPath + File.separator + "Throughput_bits.txt",
						true));
				out.write(printTime + "\t\t");
				for (int i = 0; i < nmbrOfNodes; i++) {
					out.write(Integer
							.toString((int) getThrBps(((pamvotis.core.MobileNode) nodesList
									.elementAt(i)).params.id))
							+ "\t");
				}
				out.write(Integer.toString((int) getSysThrBps()) + "\r\n");
				out.close();
			}
			if (outResults.contains("tp")) {
				out = new java.io.BufferedWriter(
						new java.io.FileWriter(resultsPath + File.separator
								+ "Throughput_Packets.txt", true));
				out.write(printTime + "\t\t");
				for (int i = 0; i < nmbrOfNodes; i++) {
					out.write(Integer
							.toString((int) getThrPkts(((pamvotis.core.MobileNode) nodesList
									.elementAt(i)).params.id))
							+ "\t");
				}
				out.write(Integer.toString((int) getSysThrPkts()) + "\r\n");
				out.close();
			}
			if (outResults.contains("ut")) {
				out = new java.io.BufferedWriter(new java.io.FileWriter(
						resultsPath + File.separator + "Utilization.txt", true));
				out.write(printTime + "\t\t");
				for (int i = 0; i < nmbrOfNodes; i++) {
					result = (float) (Math
							.round(getUtil(((pamvotis.core.MobileNode) nodesList
									.elementAt(i)).params.id) * 10000) / 10000d);
					out.write(Float.toString(result) + "\t");
				}
				result = (float) (Math.round(getSysUtil() * 10000) / 10000d);
				out.write(Float.toString(result) + "\r\n");
				out.close();
			}
			if (outResults.contains("md")) {
				out = new java.io.BufferedWriter(
						new java.io.FileWriter(resultsPath + File.separator
								+ "Media_Access_Delay.txt", true));
				out.write(printTime + "\t\t");
				for (int i = 0; i < nmbrOfNodes; i++) {
					result = (float) (Math
							.round(getMDelay(((pamvotis.core.MobileNode) nodesList
									.elementAt(i)).params.id) * 100) / 100d);
					out.write(Float.toString(result) + "\t");
				}
				out.write("\r\n");
				out.close();
			}
			if (outResults.contains("qd")) {
				out = new java.io.BufferedWriter(new java.io.FileWriter(
						resultsPath + File.separator + "Queuing_Delay.txt",
						true));
				out.write(printTime + "\t\t");
				for (int i = 0; i < nmbrOfNodes; i++) {
					result = (float) (Math
							.round(getQDelay(((pamvotis.core.MobileNode) nodesList
									.elementAt(i)).params.id) * 100) / 100d);
					out.write(Float.toString(result) + "\t");
				}
				out.write("\r\n");
				out.close();
			}
			if (outResults.contains("td")) {
				out = new java.io.BufferedWriter(new java.io.FileWriter(
						resultsPath + File.separator + "Total_Delay.txt", true));
				out.write(printTime + "\t\t");
				for (int i = 0; i < nmbrOfNodes; i++) {
					result = (float) (Math
							.round(getDelay(((pamvotis.core.MobileNode) nodesList
									.elementAt(i)).params.id) * 100) / 100d);
					out.write(Float.toString(result) + "\t");
				}
				out.write("\r\n");
				out.close();
			}
			if (outResults.contains("dj")) {
				out = new java.io.BufferedWriter(new java.io.FileWriter(
						resultsPath + File.separator + "Jitter.txt", true));
				out.write(printTime + "\t\t");
				for (int i = 0; i < nmbrOfNodes; i++) {
					result = (float) (Math
							.round(getJitter(((pamvotis.core.MobileNode) nodesList
									.elementAt(i)).params.id) * 100) / 100d);
					out.write(Float.toString(result) + "\t");
				}
				out.write("\r\n");
				out.close();
			}
			if (outResults.contains("ql")) {
				out = new java.io.BufferedWriter(
						new java.io.FileWriter(resultsPath + File.separator
								+ "Queue_Length.txt", true));
				out.write(printTime + "\t\t");
				for (int i = 0; i < nmbrOfNodes; i++) {
					out.write(Integer
							.toString((int) getQLength(((pamvotis.core.MobileNode) nodesList
									.elementAt(i)).params.id))
							+ "\t");
				}
				out.write("\r\n");
				out.close();
			}
			if (outResults.contains("ra")) {
				out = new java.io.BufferedWriter(new java.io.FileWriter(
						resultsPath + File.separator
								+ "Retransmission_Attempts.txt", true));
				out.write(printTime + "\t\t");
				for (int i = 0; i < nmbrOfNodes; i++) {
					result = (float) (Math
							.round(getRatts(((pamvotis.core.MobileNode) nodesList
									.elementAt(i)).params.id) * 1000) / 1000d);
					out.write(Float.toString(result) + "\t");
				}
				out.write("\r\n");
				out.close();
			}
		} catch (java.io.IOException e) {
			e.printStackTrace();
		} catch (pamvotis.exceptions.ElementDoesNotExistException e) {
			e.printStackTrace();
			System.exit(1);
		}
	}

	private int padBits(int rate, int psdu) {
		int NDBPS = 0;
		int NSYM = 0;
		int NDATA = 0;
		switch (rate) {
		case 6000000:
			NDBPS = 24;

		case 9000000:
			NDBPS = 36;

		case 12000000:
			NDBPS = 48;

		case 18000000:
			NDBPS = 72;

		case 24000000:
			NDBPS = 96;

		case 36000000:
			NDBPS = 144;

		case 48000000:
			NDBPS = 192;

		case 54000000:
			NDBPS = 216;

		default:
			NDBPS = 1;

		}
		NSYM = (int) Math.ceil((double) (16 + psdu + 6) / (double) NDBPS);
		NDATA = NSYM * NDBPS;
		return NDATA - (16 + psdu + 6);
	}

	private void resetResultCounters() {
		for (int i = 0; i < nmbrOfNodes; i++) {
			((pamvotis.core.MobileNode) nodesList.elementAt(i)).successfulBits = 0;
			((pamvotis.core.MobileNode) nodesList.elementAt(i)).transmissionDuration = 0;
			((pamvotis.core.MobileNode) nodesList.elementAt(i)).successfulTransmissions = 0;
			((pamvotis.core.MobileNode) nodesList.elementAt(i)).queuingDelay = 0;
			((pamvotis.core.MobileNode) nodesList.elementAt(i)).jitter = 0;
			((pamvotis.core.MobileNode) nodesList.elementAt(i)).queueLength = 0;
			((pamvotis.core.MobileNode) nodesList.elementAt(i)).collisions = 0;
		}
	}

	private void freeze() {
		int thisDur = 0;
		pamvotis.core.MobileNode n = null;
		transTimeRemaining--;
		if (transTimeRemaining == 0) {
			transmissionPending = false;
			transmitWithRTS = false;
			for (int i = 0; i < nmbrOfNodes; i++) {
				n = (pamvotis.core.MobileNode) (pamvotis.core.MobileNode) nodesList
						.elementAt(i);
				n.nowTransmitting = false;
				if (n.successfullyTransmitting == true) {
					n.successfullyTransmitting = false;
					n.successfulTransmissions++;
					n.successfulBits += n.pktLength;
					n.transmissionDuration += MobileNode.timer
							- n.transmissionStart + 1;
					thisDur = (int) (MobileNode.timer - n.getPacketBuffer()
							.firstPacket().generationTime);
					n.jitter += (int) Math.pow((double) thisDur, (double) 2);
					n.getPacketBuffer().dequeue();
					n.havePktToSend = false;
					n.lastPktTrans = MobileNode.timer;
				}
			}
		}
	}

	private void takePacketFromQueue(int i) {
		int idleDur = 0;
		pamvotis.core.MobileNode n = (pamvotis.core.MobileNode) (pamvotis.core.MobileNode) nodesList
				.elementAt(i);
		n.pktLength = n.getPacketBuffer().firstPacket().length;
		n.queuingDelay += MobileNode.timer
				- n.getPacketBuffer().firstPacket().generationTime;
		idleDur = (int) (MobileNode.timer - n.lastPktTrans);
		if (idleDur > (int) (n.params.aifsd / slot)) {
			n.contWind = n.params.cwMin;
			n.backoffCounter = 0;
		} else {
			n.contWind = n.params.cwMin;
			n.backoffCounter = n.InitBackoff(n.params.cwMin);
		}
		n.transmissionStart = MobileNode.timer;
		n.havePktToSend = true;
	}

	private void updateMeanResults() {
		pamvotis.core.MobileNode n = null;
		for (int i = 0; i < nmbrOfNodes; i++) {
			n = (pamvotis.core.MobileNode) nodesList.elementAt(i);
			n.totCollisions += n.collisions;
			n.totJitter += n.jitter;
			n.totQueueLength += n.queueLength;
			n.totQueuingDelay += n.queuingDelay;
			n.totSuccessfulBits += n.successfulBits;
			n.totSuccessfulTransmissions += n.successfulTransmissions;
			n.totTransmissionDurations += n.transmissionDuration;
		}
	}

	public pamvotis.core.MobileNode getNode(int nodeId)
			throws pamvotis.exceptions.ElementDoesNotExistException {
		pamvotis.core.MobileNode nd = null;
		for (int i = 0; i < nodesList.size(); i++) {
			if (((pamvotis.core.MobileNode) nodesList.elementAt(i)).params.id == nodeId) {
				nd = (pamvotis.core.MobileNode) nodesList.elementAt(i);
				break;
			}
		}
		if (nd == null) {
			throw new pamvotis.exceptions.ElementDoesNotExistException("Node "
					+ nodeId + " does not exist.");
		}
		return nd;
	}

	public void simulate(long startTime, long endTime) {
		pamvotis.core.MobileNode n = null;
		resetResultCounters();
		simTime = (long) ((endTime - startTime + 1) / slot / 1000);
		startTime = (long) ((startTime - 1) / slot / 1000 + 1);
		endTime = (long) (endTime / slot / 1000);
		Vector<Packet> newPackets = null;
		for (long currentSlot = startTime; currentSlot <= endTime; currentSlot++) {
			MobileNode.timer = currentSlot;
			Source.timer = currentSlot;
			for (int i = 0; i < nmbrOfNodes; i++) {
				n = (pamvotis.core.MobileNode) (pamvotis.core.MobileNode) nodesList
						.elementAt(i);
				for (int j = 0; j < n._srcManager._vActiveSources.size(); j++) {
					((pamvotis.sources.Source) n._srcManager._vActiveSources
							.elementAt(j)).synchronize();
				}
				newPackets = n.pollPacketsFromSources();
				if (!newPackets.isEmpty()) {
					n.getPacketBuffer().enqueue(newPackets);
				}
				if (n.havePktToSend == false
						&& n.getPacketBuffer().isEmpty() == false) {
					takePacketFromQueue(i);
				}
				n.queueLength += (float) n.getPacketBuffer().size();
			}
			fightForSlot();
			currentTime = (float) (currentSlot * slot);
			progress = (short) (currentSlot * slot / (float) totalTime * 100);
			if (currentTime == (float) totalTime) {
				progress = 100;
			}
		}
		updateMeanResults();
		printStats();
	}

	public void printHeaders() {
		try {
			if (outResults.contains("tb")) {
				out = new java.io.BufferedWriter(new java.io.FileWriter(
						resultsPath + File.separator + "Throughput_bits.txt"));
				out.write("\t\t\t*****\t Throughput (Kbits/s)\t*****\r\n\r\n\r\n");
				out.write("Time (sec)\t");
				for (int i = 1; i <= nmbrOfNodes; i++) {
					out.write("Node " + i + "\t");
				}
				out.write("System\r\n");
				out.close();
			}
			if (outResults.contains("tp")) {
				out = new java.io.BufferedWriter(
						new java.io.FileWriter(resultsPath + File.separator
								+ "Throughput_Packets.txt"));
				out.write("\t\t\t*****\t Throughput (packets/s)\t*****\r\n\r\n\r\n");
				out.write("Time (sec)\t");
				for (int i = 1; i <= nmbrOfNodes; i++) {
					out.write("Node " + i + "\t");
				}
				out.write("System\r\n");
				out.close();
			}
			if (outResults.contains("ut")) {
				out = new java.io.BufferedWriter(new java.io.FileWriter(
						resultsPath + File.separator + "Utilization.txt"));
				out.write("\t\t\t*****\t Utilization\t*****\r\n\r\n\r\n");
				out.write("Time (sec)\t");
				for (int i = 1; i <= nmbrOfNodes; i++) {
					out.write("Node " + i + "\t");
				}
				out.write("System\r\n");
				out.close();
			}
			if (outResults.contains("md")) {
				out = new java.io.BufferedWriter(
						new java.io.FileWriter(resultsPath + File.separator
								+ "Media_Access_Delay.txt"));
				out.write("\t\t\t*****\t Media Access Delay (msec) \t*****\r\n\r\n\r\n");
				out.write("Time (sec)\t");
				for (int i = 1; i <= nmbrOfNodes; i++) {
					out.write("Node " + i + "\t");
				}
				out.write("\r\n");
				out.close();
			}
			if (outResults.contains("qd")) {
				out = new java.io.BufferedWriter(new java.io.FileWriter(
						resultsPath + File.separator + "Queuing_Delay.txt"));
				out.write("\t\t\t*****\t Queuing Delay (msec) \t*****\r\n\r\n\r\n");
				out.write("Time (sec)\t");
				for (int i = 1; i <= nmbrOfNodes; i++) {
					out.write("Node " + i + "\t");
				}
				out.write("\r\n");
				out.close();
			}
			if (outResults.contains("td")) {
				out = new java.io.BufferedWriter(new java.io.FileWriter(
						resultsPath + File.separator + "Total_Delay.txt"));
				out.write("\t\t\t*****\t Total Packet Delay (msec) \t*****\r\n\r\n\r\n");
				out.write("Time (sec)\t");
				for (int i = 1; i <= nmbrOfNodes; i++) {
					out.write("Node " + i + "\t");
				}
				out.write("\r\n");
				out.close();
			}
			if (outResults.contains("dj")) {
				out = new java.io.BufferedWriter(new java.io.FileWriter(
						resultsPath + File.separator + "Jitter.txt"));
				out.write("\t\t\t*****\t Delay Jitter (msec) \t*****\r\n\r\n\r\n");
				out.write("Time (sec)\t");
				for (int i = 1; i <= nmbrOfNodes; i++) {
					out.write("Node " + i + "\t");
				}
				out.write("\r\n");
				out.close();
			}
			if (outResults.contains("ql")) {
				out = new java.io.BufferedWriter(new java.io.FileWriter(
						resultsPath + File.separator + "Queue_Length.txt"));
				out.write("\t\t\t*****\t Packet Queue Length\t*****\r\n\r\n\r\n");
				out.write("Time (sec)\t");
				for (int i = 1; i <= nmbrOfNodes; i++) {
					out.write("Node " + i + "\t");
				}
				out.write("\r\n");
				out.close();
			}
			if (outResults.contains("ra")) {
				out = new java.io.BufferedWriter(new java.io.FileWriter(
						resultsPath + File.separator
								+ "Retransmission_Attempts.txt"));
				out.write("\t\t\t*****\t Retransmission Attempts\t*****\r\n\r\n\r\n");
				out.write("Time (sec)\t");
				for (int i = 1; i <= nmbrOfNodes; i++) {
					out.write("Node " + i + "\t");
				}
				out.write("\r\n");
				out.close();
			}
		} catch (java.io.IOException e) {
			e.printStackTrace();
		}
	}

	public void printMeanValues() {
		float thrBt = 0;
		float thrPkt = 0;
		float util = 0;
		float mDel = 0;
		float qDel = 0;
		float tDel = 0;
		float jitter = 0;
		float rAtts = 0;
		float qLngth = 0;
		float thrTotBt = 0;
		float thrTotPkt = 0;
		float utilTot = 0;
		try {
			out = new java.io.BufferedWriter(new java.io.FileWriter(resultsPath
					+ File.separator + "Mean_Values.txt"));
			out.write("\t\t\t*****\t Mean Statistic Values\t*****\r\n\r\n");
			out.write("Node\tThroughput\tThroughput\t");
			out.write("Utilization\tMedia Access Delay\t");
			out.write("Queuing Delay\tTotal Packet Delay\tDelay Jitter\t");
			out.write("Queue Length\tRetransmission Attempts\r\n");
			out.write("\t(Kbits/s)\t(packets/s)\t\t\t(msec)\t\t\t(msec)");
			out.write("\t\t(msec)\t\t\t(msec)\r\n");
			for (int i = 0; i < nmbrOfNodes; i++) {
				thrBt = (float) ((pamvotis.core.MobileNode) nodesList
						.elementAt(i)).totSuccessfulBits
						/ (float) totalTime
						/ 1000;
				thrTotBt += thrBt;
				thrPkt = (float) ((pamvotis.core.MobileNode) nodesList
						.elementAt(i)).totSuccessfulTransmissions
						/ (float) totalTime;
				thrTotPkt += thrPkt;
				util = (float) thrBt
						* 1000
						/ (float) ((pamvotis.core.MobileNode) nodesList
								.elementAt(i)).params.rate;
				utilTot += util;
				mDel = (float) ((pamvotis.core.MobileNode) nodesList
						.elementAt(i)).totTransmissionDurations
						/ (float) ((pamvotis.core.MobileNode) nodesList
								.elementAt(i)).totSuccessfulTransmissions;
				mDel = mDel * slot * 1000;
				qDel = (float) ((pamvotis.core.MobileNode) nodesList
						.elementAt(i)).totQueuingDelay
						/ (float) ((pamvotis.core.MobileNode) nodesList
								.elementAt(i)).totSuccessfulTransmissions;
				qDel = qDel * slot * 1000;
				tDel = mDel + qDel;
				jitter = (float) ((pamvotis.core.MobileNode) nodesList
						.elementAt(i)).totJitter
						/ (float) ((pamvotis.core.MobileNode) nodesList
								.elementAt(i)).totSuccessfulTransmissions
						- (float) Math.pow((float) tDel, 2);
				jitter = (float) Math.sqrt((float) jitter);
				jitter = jitter * slot * 1000;
				qLngth = (float) ((pamvotis.core.MobileNode) nodesList
						.elementAt(i)).totQueueLength
						/ (float) totalTime
						* slot;
				rAtts = (float) ((pamvotis.core.MobileNode) nodesList
						.elementAt(i)).totCollisions
						/ (float) ((pamvotis.core.MobileNode) nodesList
								.elementAt(i)).totSuccessfulTransmissions;
				thrBt = (float) Math.round(thrBt);
				thrPkt = (float) Math.round(thrPkt);
				util = (float) (Math.round(util * 10000) / 10000d);
				mDel = (float) (Math.round(mDel * 100) / 100d);
				qDel = (float) (Math.round(qDel * 100) / 100d);
				tDel = (float) (Math.round(tDel * 100) / 100d);
				jitter = (float) (Math.round(jitter * 100) / 100d);
				rAtts = (float) (Math.round(rAtts * 1000) / 1000d);
				out.write(i + 1 + "\t" + (int) thrBt + "\t\t");
				out.write((int) thrPkt + "\t\t" + util + "\t\t");
				out.write(mDel + "\t\t\t" + qDel + "\t\t" + tDel + "\t\t\t");
				out.write(jitter + "\t\t" + (int) qLngth + "\t\t" + rAtts
						+ "\r\n");
			}
			thrTotBt = (float) Math.round(thrTotBt);
			thrTotPkt = (float) Math.round(thrTotPkt);
			utilTot = (float) (Math.round(utilTot * 10000) / 10000d);
			out.write("\r\nSystem\t" + (int) thrTotBt + "\t\t");
			out.write((int) thrTotPkt + "\t\t" + utilTot + "\t\t");
			out.close();
		} catch (java.io.IOException e) {
			e.printStackTrace();
		}
	}

	public int getProgress() {
		return progress;
	}

	public long getTime() {
		return Math.round(currentTime);
	}

	public float getThrBps(int node)
			throws pamvotis.exceptions.ElementDoesNotExistException {
		return (float) (getNode(node).successfulBits / (simTime * slot * 1000));
	}

	public float getSysThrBps() {
		float result = 0;
		for (int i = 0; i < nmbrOfNodes; i++) {
			result += (float) (((pamvotis.core.MobileNode) nodesList
					.elementAt(i)).successfulBits / (simTime * slot * 1000));
		}
		return result;
	}

	public float getThrPkts(int node)
			throws pamvotis.exceptions.ElementDoesNotExistException {
		return (float) (getNode(node).successfulTransmissions / (double) simTime)
				/ slot;
	}

	public float getSysThrPkts() {
		float result = 0;
		for (int i = 0; i < nmbrOfNodes; i++) {
			result += (float) (((pamvotis.core.MobileNode) nodesList
					.elementAt(i)).successfulTransmissions / (double) simTime)
					/ slot;
		}
		return result;
	}

	public float getUtil(int node)
			throws pamvotis.exceptions.ElementDoesNotExistException {
		return (float) (getNode(node).successfulBits / (simTime * slot * getNode(node).params.rate));
	}

	public float getSysUtil() {
		float resultTot = 0;
		for (int i = 0; i < nmbrOfNodes; i++) {
			resultTot += (float) (((pamvotis.core.MobileNode) nodesList
					.elementAt(i)).successfulBits / (simTime * slot * ((pamvotis.core.MobileNode) nodesList
					.elementAt(i)).params.rate));
		}
		return resultTot;
	}

	public float getMDelay(int node)
			throws pamvotis.exceptions.ElementDoesNotExistException {
		try {
			float result = (float) (getNode(node).transmissionDuration / getNode(node).successfulTransmissions);
			return result * slot * 1000;
		} catch (java.lang.ArithmeticException e) {
			return 0;
		}
	}

	public float getQDelay(int node)
			throws pamvotis.exceptions.ElementDoesNotExistException {
		try {
			float result = (float) (getNode(node).queuingDelay / getNode(node).successfulTransmissions);
			return result * slot * 1000;
		} catch (java.lang.ArithmeticException e) {
			return 0;
		}
	}

	public float getDelay(int node)
			throws pamvotis.exceptions.ElementDoesNotExistException {
		return getMDelay(node) + getQDelay(node);
	}

	public float getJitter(int node)
			throws pamvotis.exceptions.ElementDoesNotExistException {
		try {
			float result = (float) (getNode(node).jitter / getNode(node).successfulTransmissions)
					- (float) Math.pow((float) getDelay(node), 2);
			result = (float) Math.sqrt((float) result);
			result = (float) (result * slot * 1000);
			return result;
		} catch (java.lang.ArithmeticException e) {
			return 0;
		}
	}

	public float getQLength(int node)
			throws pamvotis.exceptions.ElementDoesNotExistException {
		return (float) (getNode(node).queueLength / simTime);
	}

	public float getRatts(int node)
			throws pamvotis.exceptions.ElementDoesNotExistException {
		try {
			return (float) getNode(node).collisions
					/ (float) getNode(node).successfulTransmissions;
		} catch (java.lang.ArithmeticException e) {
			return 0;
		}
	}

	public void changeNodeParams(int node, int coverage, int xPosition,
			int yPosition) {
		pamvotis.core.MobileNode n = (pamvotis.core.MobileNode) nodesList
				.elementAt(node);
		if (n == null) {
			return;
		}
		if (coverage != -1) {
			n.params.coverage = coverage;
		}
		if (xPosition != -1) {
			n.params.x = xPosition;
		}
		if (yPosition != -1) {
			n.params.y = yPosition;
		}
	}

	public void confParams() {
		try {
			javax.xml.parsers.DocumentBuilder db = DocumentBuilderFactory
					.newInstance().newDocumentBuilder();
			org.w3c.dom.Document doc = db.parse("config" + File.separator
					+ "NtConf.xml");
			seed = Integer.parseInt(doc.getElementsByTagName("seed").item(0)
					.getTextContent());
			totalTime = Integer.parseInt(doc.getElementsByTagName("duration")
					.item(0).getTextContent());
			mixNodes = Integer.parseInt(doc.getElementsByTagName("mixNodes")
					.item(0).getTextContent());
			rtsThr = Integer.parseInt(doc.getElementsByTagName("RTSThr")
					.item(0).getTextContent());
			ctsToSelf = doc.getElementsByTagName("ctsToSelf").item(0)
					.getTextContent().charAt(0);
			phyLayer = doc.getElementsByTagName("phyLayer").item(0)
					.getTextContent().charAt(0);
			resultsPath = doc.getElementsByTagName("resultsPath").item(0)
					.getTextContent();
			outResults = doc.getElementsByTagName("outResults").item(0)
					.getTextContent();
			if (phyLayer == 'a' || phyLayer == 'g') {
				slot = (float) SpecParams.SLOT_ERP;
				cwMin = SpecParams.CW_MIN_OFDM;
			} else {
				slot = (float) SpecParams.SLOT_NON_ERP;
				cwMin = SpecParams.CW_MIN_DSSS;
			}
			if (phyLayer == 'a') {
				sifs = (float) SpecParams.SIFS_A;
			} else {
				sifs = (float) SpecParams.SIFS_G;
			}
			cwMinFact0 = Integer.parseInt(doc
					.getElementsByTagName("cwMinFact0").item(0)
					.getTextContent());
			cwMinFact1 = Integer.parseInt(doc
					.getElementsByTagName("cwMinFact1").item(0)
					.getTextContent());
			cwMinFact2 = Integer.parseInt(doc
					.getElementsByTagName("cwMinFact2").item(0)
					.getTextContent());
			cwMinFact3 = Integer.parseInt(doc
					.getElementsByTagName("cwMinFact3").item(0)
					.getTextContent());
			cwMaxFact0 = Integer.parseInt(doc
					.getElementsByTagName("cwMaxFact0").item(0)
					.getTextContent());
			cwMaxFact1 = Integer.parseInt(doc
					.getElementsByTagName("cwMaxFact1").item(0)
					.getTextContent());
			cwMaxFact2 = Integer.parseInt(doc
					.getElementsByTagName("cwMaxFact2").item(0)
					.getTextContent());
			cwMaxFact3 = Integer.parseInt(doc
					.getElementsByTagName("cwMaxFact3").item(0)
					.getTextContent());
			aifs0 = Integer.parseInt(doc.getElementsByTagName("aifs0").item(0)
					.getTextContent());
			aifs1 = Integer.parseInt(doc.getElementsByTagName("aifs1").item(0)
					.getTextContent());
			aifs2 = Integer.parseInt(doc.getElementsByTagName("aifs2").item(0)
					.getTextContent());
			aifs3 = Integer.parseInt(doc.getElementsByTagName("aifs3").item(0)
					.getTextContent());
			org.w3c.dom.NodeList nodes = doc.getElementsByTagName("node");
			nmbrOfNodes = 0;
			generator = new java.util.Random((long) seed);
			RandomGenerator.generator = generator;
			MobileNode.generator = generator;
			Source.slot = slot;
			for (int i = 0; i < nodes.getLength(); i++) {
				org.w3c.dom.Node node = nodes.item(i);
				org.w3c.dom.Element ndElement = (org.w3c.dom.Element) node;
				org.w3c.dom.NamedNodeMap attrs = node.getAttributes();
				int id = Integer.parseInt(attrs.getNamedItem("number")
						.getNodeValue());
				int rate = Integer.parseInt(ndElement
						.getElementsByTagName("rate").item(0).getTextContent());
				int coverage = Integer.parseInt(ndElement
						.getElementsByTagName("coverage").item(0)
						.getTextContent());
				int xPosition = Integer.parseInt(ndElement
						.getElementsByTagName("xPosition").item(0)
						.getTextContent());
				int yPosition = Integer.parseInt(ndElement
						.getElementsByTagName("yPosition").item(0)
						.getTextContent());
				int ac = Integer.parseInt(ndElement.getElementsByTagName("AC")
						.item(0).getTextContent());
				try {
					addNode(id, rate, coverage, xPosition, yPosition, ac);
				} catch (pamvotis.exceptions.ElementExistsException e) {
					throw new pamvotis.exceptions.ConfigurationException(
							"You have already configured a node with ID "
									+ id
									+ ". Please check your network configuration file.");
				}
				org.w3c.dom.NodeList sourceList = ndElement
						.getElementsByTagName("source");
				for (int j = 0; j < sourceList.getLength(); j++) {
					org.w3c.dom.Node source = sourceList.item(j);
					org.w3c.dom.Element sourceElement = (org.w3c.dom.Element) source;
					org.w3c.dom.NamedNodeMap attributes = source
							.getAttributes();
					int sourceId = Integer.parseInt(attributes.getNamedItem(
							"id").getNodeValue());
					try {
						if (attributes.getNamedItem("type").getNodeValue()
								.equals("generic")) {
							float pktLnght = Float.parseFloat(sourceElement
									.getElementsByTagName("pktLngth").item(0)
									.getTextContent());
							float intArr = Float.parseFloat(sourceElement
									.getElementsByTagName("intArrTime").item(0)
									.getTextContent());
							char pktDist = sourceElement
									.getElementsByTagName("pktDist").item(0)
									.getTextContent().charAt(0);
							char intArrDstr = sourceElement
									.getElementsByTagName("intArrDstr").item(0)
									.getTextContent().charAt(0);
							pamvotis.sources.GenericSource s = null;
							try {
								s = new pamvotis.sources.GenericSource(
										sourceId, intArrDstr, intArr, pktDist,
										pktLnght);
							} catch (pamvotis.exceptions.UnknownDistributionException ex) {
								throw new pamvotis.exceptions.ConfigurationException(
										"The packet length or/and the packet interarrival time distribution(s) you have configured are invalid. Only 'c','u' and 'e' are allowed.");
							}
							appendNewSource(id, s);
						} else {
							if (attributes.getNamedItem("type").getNodeValue()
									.equals("ftp")) {
								int pktSize = Integer.parseInt(sourceElement
										.getElementsByTagName("pktSize")
										.item(0).getTextContent());
								float fileSizeMean = Float
										.parseFloat(sourceElement
												.getElementsByTagName(
														"fileSizeMean").item(0)
												.getTextContent());
								float fileSizeStDev = Float
										.parseFloat(sourceElement
												.getElementsByTagName(
														"fileSizeStDev")
												.item(0).getTextContent());
								float fileSizeMax = Float
										.parseFloat(sourceElement
												.getElementsByTagName(
														"fileSizeMax").item(0)
												.getTextContent());
								float readingTime = Float
										.parseFloat(sourceElement
												.getElementsByTagName(
														"readingTime").item(0)
												.getTextContent());
								pamvotis.sources.FTPSource s = new pamvotis.sources.FTPSource(
										sourceId, pktSize, fileSizeMean,
										fileSizeStDev, fileSizeMax, readingTime);
								appendNewSource(id, s);
							} else {
								if (attributes.getNamedItem("type")
										.getNodeValue().equals("video")) {
									int frameRate = Integer
											.parseInt(sourceElement
													.getElementsByTagName(
															"frameRate")
													.item(0).getTextContent());
									int packetsPerFrame = Integer
											.parseInt(sourceElement
													.getElementsByTagName(
															"packetsPerFrame")
													.item(0).getTextContent());
									float pktSize = Float
											.parseFloat(sourceElement
													.getElementsByTagName(
															"pktSize").item(0)
													.getTextContent());
									float pktSizeMax = Float
											.parseFloat(sourceElement
													.getElementsByTagName(
															"pktSizeMax")
													.item(0).getTextContent());
									float pktIntArr = Float
											.parseFloat(sourceElement
													.getElementsByTagName(
															"pktIntArr")
													.item(0).getTextContent());
									float pktIntArrMax = Float
											.parseFloat(sourceElement
													.getElementsByTagName(
															"pktIntArrMax")
													.item(0).getTextContent());
									pamvotis.sources.VideoSource s = new pamvotis.sources.VideoSource(
											sourceId, frameRate,
											packetsPerFrame, pktSize,
											pktSizeMax, pktIntArr, pktIntArrMax);
									appendNewSource(id, s);
								} else {
									if (attributes.getNamedItem("type")
											.getNodeValue().equals("http")) {
										int pktSize = Integer
												.parseInt(sourceElement
														.getElementsByTagName(
																"pktSize")
														.item(0)
														.getTextContent());
										float mainObjectMean = Float
												.parseFloat(sourceElement
														.getElementsByTagName(
																"mainObjectMean")
														.item(0)
														.getTextContent());
										float mainObjectStDev = Float
												.parseFloat(sourceElement
														.getElementsByTagName(
																"mainObjectStDev")
														.item(0)
														.getTextContent());
										float mainObjectMin = Float
												.parseFloat(sourceElement
														.getElementsByTagName(
																"mainObjectMin")
														.item(0)
														.getTextContent());
										float mainObjectMax = Float
												.parseFloat(sourceElement
														.getElementsByTagName(
																"mainObjectMax")
														.item(0)
														.getTextContent());
										float embObjectMean = Float
												.parseFloat(sourceElement
														.getElementsByTagName(
																"embObjectMean")
														.item(0)
														.getTextContent());
										float embObjectStDev = Float
												.parseFloat(sourceElement
														.getElementsByTagName(
																"embObjectStDev")
														.item(0)
														.getTextContent());
										float embObjectMin = Float
												.parseFloat(sourceElement
														.getElementsByTagName(
																"embObjectMin")
														.item(0)
														.getTextContent());
										float embObjectMax = Float
												.parseFloat(sourceElement
														.getElementsByTagName(
																"embObjectMax")
														.item(0)
														.getTextContent());
										float NumOfEmbObjectsMean = Float
												.parseFloat(sourceElement
														.getElementsByTagName(
																"NumOfEmbObjectsMean")
														.item(0)
														.getTextContent());
										float NumOfEmbObjectsMax = Float
												.parseFloat(sourceElement
														.getElementsByTagName(
																"NumOfEmbObjectsMax")
														.item(0)
														.getTextContent());
										float readingTime = Float
												.parseFloat(sourceElement
														.getElementsByTagName(
																"readingTime")
														.item(0)
														.getTextContent());
										float parsingTime = Float
												.parseFloat(sourceElement
														.getElementsByTagName(
																"parsingTime")
														.item(0)
														.getTextContent());
										pamvotis.sources.HTTPSource s = new pamvotis.sources.HTTPSource(
												sourceId, pktSize,
												mainObjectMean,
												mainObjectStDev, mainObjectMin,
												mainObjectMax, embObjectMean,
												embObjectStDev, embObjectMin,
												embObjectMax,
												NumOfEmbObjectsMean,
												NumOfEmbObjectsMax,
												readingTime, parsingTime);
										appendNewSource(id, s);
									}
								}
							}
						}
					} catch (pamvotis.exceptions.ElementExistsException ex) {
						throw new pamvotis.exceptions.ConfigurationException(
								"You have already configured a source with ID "
										+ sourceId
										+ ". Check your network configuration file.");
					} catch (pamvotis.exceptions.ElementDoesNotExistException ex) {
						throw new pamvotis.exceptions.ConfigurationException(
								"Node "
										+ id
										+ ", where you are trying to add source "
										+ sourceId
										+ ", already exists. Please check your network configuration file.");
					}
				}
			}
		} catch (java.lang.NumberFormatException e) {
			e.printStackTrace();
		} catch (org.w3c.dom.DOMException e) {
			e.printStackTrace();
		} catch (javax.xml.parsers.ParserConfigurationException e) {
			e.printStackTrace();
		} catch (org.xml.sax.SAXException e) {
			e.printStackTrace();
		} catch (java.io.IOException e) {
			e.printStackTrace();
		} catch (pamvotis.exceptions.ConfigurationException e) {
			e.printStackTrace();
			// System.exit(1);
		}
	}

	public void addNode(int id, int rate, int coverage, int xPosition,
			int yPosition, int ac)
			throws pamvotis.exceptions.ElementExistsException {
		boolean nodeExists = false;
		for (int i = 0; i < nodesList.size(); i++) {
			if (((pamvotis.core.MobileNode) nodesList.elementAt(i)).params.id == id) {
				nodeExists = true;
				break;
			}
		}
		if (nodeExists) {
			throw new pamvotis.exceptions.ElementExistsException("Node " + id
					+ " already exists.");
		} else {
			pamvotis.core.MobileNode nd = new pamvotis.core.MobileNode();
			int nCwMin = cwMin;
			int nCwMax = SpecParams.CW_MAX;
			float nAifsd = sifs + 2 * slot;
			switch (ac) {
			case 1: {
				nCwMin = (int) ((float) cwMin / (float) cwMinFact1);
				nCwMax = (int) ((float) SpecParams.CW_MAX / (float) cwMaxFact1);
				nAifsd = sifs + aifs1 * slot;
				break;
			}

			case 2: {
				nCwMin = (int) ((float) cwMin / (float) cwMinFact2);
				nCwMax = (int) ((float) SpecParams.CW_MAX / (float) cwMaxFact2);
				nAifsd = sifs + aifs2 * slot;
				break;
			}

			case 3: {
				nCwMin = (int) ((float) cwMin / (float) cwMinFact3);
				nCwMax = (int) ((float) SpecParams.CW_MAX / (float) cwMaxFact3);
				nAifsd = sifs + aifs3 * slot;
				break;
			}

			default: {
				nCwMin = (int) ((float) cwMin / (float) cwMinFact0);
				nCwMax = (int) ((float) SpecParams.CW_MAX / (float) cwMaxFact0);
				nAifsd = sifs + aifs0 * slot;
				break;
			}

			}
			nd.params.InitParams(id, rate, xPosition, yPosition, coverage, ac,
					nAifsd, nCwMin, nCwMax);
			nd.contWind = nd.params.cwMin;
			nodesList.addElement(nd);
			nmbrOfNodes++;
		}
	}

	public boolean removeNode(int nodeId)
			throws pamvotis.exceptions.ElementDoesNotExistException {
		int position = -1;
		for (int i = 0; i < nodesList.size(); i++) {
			if (((pamvotis.core.MobileNode) nodesList.elementAt(i)).params.id == nodeId) {
				position = i;
				break;
			}
		}
		if (position != -1) {
			nodesList.removeElementAt(position);
			nmbrOfNodes--;
			return true;
		} else {
			throw new pamvotis.exceptions.ElementDoesNotExistException("Node "
					+ nodeId + " does not exist.");
		}
	}

	public void removeAllNodes() {
		nodesList.clear();
		nmbrOfNodes = 0;
	}

	public boolean appendNewSource(int node, pamvotis.sources.Source newSource)
			throws pamvotis.exceptions.ElementExistsException,
			pamvotis.exceptions.ElementDoesNotExistException {
		pamvotis.core.MobileNode n = (pamvotis.core.MobileNode) getNode(node);
		if (n == null) {
			return false;
		}
		n.addSource(newSource);
		return true;
	}

	public boolean removeAllSources(int node)
			throws pamvotis.exceptions.ElementDoesNotExistException {
		pamvotis.core.MobileNode n = (pamvotis.core.MobileNode) getNode(node);
		if (n == null) {
			return false;
		}
		n.removeAllSources();
		return true;
	}

	public void removeSource(int node, int sourceId)
			throws pamvotis.exceptions.ElementDoesNotExistException {
		pamvotis.core.MobileNode n = getNode(node);
		if (n != null) {
			n.removeSource(sourceId);
		}
	}

}
