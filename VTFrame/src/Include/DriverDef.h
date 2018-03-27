#pragma once
#include <ntddk.h>
#include "../APC/APC.h"
#include "../IDT/idt.h"
#include "../CallBack/RemoveCallBack.h"
#include "../Test/Test.h"


//��Ӧ�ó���ͨ����
#define IOCTL_IO_TEST		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_IO_TEST1		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_IO_TEST2		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

//�����ͷ������ӵ�����
#pragma warning(disable:4129)
#define DEVICE_NAME L"\\Device\\Zanpo"
#define SYMBOL_LINK L"\\\DosDevices\\Zanpo"

//EPROCESS�ṹƫ��
#define SeAuditProcessCreationInfoOffset 0x390     //EPROCESS->SeAuditProcessCreationInfoOffset
#define ProcessParametersOffset 0x20			   //EPROCESS->ProcessParameters
#define ProcessParametersImagePathNameOffset 0x60  //ProcessParameters->ImagePathName
#define ProcessParametersCommandLineOffset 0x70	   //ProcessParameters->CommandLine
#define ProcessParametersWindowTitleOffset 0x0b0   //ProcessParameters->WindowTitle
#define LDROffset 0x018							   //PEB->ldr
#define InLoadOrderModuleListOffset 0x10           //ldr->InLoadOrderModuleList
#define InMemoryOrderModuleListOffset 0x20         //ldr->InMemoryOrderModuleList
#define InInitializationOrderModuleListOffset 0x30 //ldr->InInitializationOrderModuleList

//ȫ�ֱ���
PDEVICE_OBJECT pDevObj = NULL;

extern BOOLEAN int1bool;

NTSTATUS CreateDeviceAndSymbol(PDRIVER_OBJECT DriverObject)
{
	NTSTATUS status;
	UNICODE_STRING usDevName, usSymName;
	RtlInitUnicodeString(&usDevName, DEVICE_NAME);
	RtlInitUnicodeString(&usSymName, SYMBOL_LINK);

	status = IoCreateDevice(DriverObject, 0, &usDevName, FILE_DEVICE_UNKNOWN, 0, FALSE, &pDevObj);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Create Device error\n"));
		return status;
	}

	status = IoCreateSymbolicLink(&usSymName, &usDevName);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Create Symbol Link error\n"));
		return status;
	}

	return STATUS_SUCCESS;
}

NTSTATUS DeleteDeviceAndSymbol()
{
	UNICODE_STRING usSymName;
	RtlInitUnicodeString(&usSymName, SYMBOL_LINK);
	IoDeleteDevice(pDevObj);
	IoDeleteSymbolicLink(&usSymName);
	return STATUS_SUCCESS;
}

//���Ӧ�ó���򿪴������ķ������ӣ���ɲ����سɹ�
NTSTATUS CREATE_DISPATCH(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	KdPrint(("Entry CREATE_DISPATCH\n"));
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}


//��Ӧ�ó������ͨ�ŵĺ���
NTSTATUS DEVICE_CONTROL_DISPATCH(PDEVICE_OBJECT  DeviceObject, PIRP Irp)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	ULONG uControlCode, uInSize, uOutSize;
	PVOID pIoBuffer;
	PIO_STACK_LOCATION pStack;

	//��ȡ��Ҫ����Ϣ��1.������ָ�� 2.����������� 3.������
	pStack = IoGetCurrentIrpStackLocation(Irp);
	pIoBuffer = Irp->AssociatedIrp.SystemBuffer;
	uControlCode = pStack->Parameters.DeviceIoControl.IoControlCode;
	uInSize = pStack->Parameters.DeviceIoControl.InputBufferLength;
	uOutSize = pStack->Parameters.DeviceIoControl.OutputBufferLength;

	switch (uControlCode)
	{
		//����
	case IOCTL_IO_TEST:
	{
		ULONG64 cr3 = 0,cr3_fake = 0;
		cr3_fake = GetDxfCr3Fake();
		if (cr3_fake != 0)
			DbgPrint("dxf fake cr3:%llx\n");
		else
			DbgPrint("��ȡfake cr3ʧ��\n");

		if (GetDxfCr3Real((PVOID)0x00400000, (UINT32)8, &cr3))
			DbgPrint("dxf real cr3:%llx\n", cr3);
		else
			DbgPrint("��ȡreal cr3ʧ��\n");

		if (cr3 != 0 && cr3_fake != 0)
			__vmx_vmcall(VTFrame_Test, cr3, cr3_fake, 0);

		//ʹ�����̻߳ص�ʧЧ
		EnableObType(*PsProcessType,FALSE);
		EnableObType(*PsThreadType, FALSE);
		
		break;
	}

	case IOCTL_IO_TEST1: 
	{
		int1bool = FALSE;
		break;
	}
	case IOCTL_IO_TEST2:
	{
		int1bool = TRUE;
		break;
	}

	}
	if (NT_SUCCESS(status))
		Irp->IoStatus.Information = uOutSize;
	else
		Irp->IoStatus.Information = 0;


	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}


KIRQL WPOFFx64()
{
	KIRQL irql = KeRaiseIrqlToDpcLevel();
	UINT64 cr0 = __readcr0();
	cr0 &= 0xfffffffffffeffff;
	__writecr0(cr0);
	_disable();
	return irql;
}

void WPONx64(KIRQL irql)
{
	UINT64 cr0 = __readcr0();
	cr0 |= 0x10000;
	_enable();
	__writecr0(cr0);
	KeLowerIrql(irql);
}


typedef struct _KLDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY64 InLoadOrderLinks;
	ULONG64 __Undefined1;
	ULONG64 __Undefined2;
	ULONG64 __Undefined3;
	ULONG64 NonPagedDebugInfo;
	ULONG64 DllBase;
	ULONG64 EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	USHORT LoadCount;
	USHORT __Undefined5;
	ULONG64 __Undefined6;
	ULONG CheckSum;
	ULONG __padding1;
	ULONG TimeDateStamp;
	ULONG __padding2;
}KLDR_DATA_TABLE_ENTRY, *PKLDR_DATA_TABLE_ENTRY;

//typedef struct _KAPC_STATE
//{
//	LIST_ENTRY ApcListHead[2];
//	PKPROCESS Process;
//	UCHAR KernelApcInProgress;
//	UCHAR KernelApcPending;
//	UCHAR UserApcPending;
//} KAPC_STATE, *PKAPC_STATE;


VOID DriverUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS CREATE_DISPATCH(PDEVICE_OBJECT DeviceObject, PIRP  Irp);
NTSTATUS DEVICE_CONTROL_DISPATCH(PDEVICE_OBJECT DeviceObject, PIRP Irp);

//������ص�������
NTKERNELAPI NTSTATUS PsLookupProcessByProcessId(HANDLE Id, PEPROCESS *Process);
NTKERNELAPI UCHAR* PsGetProcessImageFileName(PEPROCESS process);
NTKERNELAPI PPEB PsGetProcessPeb(PEPROCESS process);


